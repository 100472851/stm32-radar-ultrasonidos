/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stdio.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define VECES_REBOTES 5 //estas veces 100us
#define NUMERO_SECCIONES 19 //para el ADC

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
char cambio_modo;
char hacer_medida;
enum modos{MANUAL, AUTOMATICO};
enum modos modo = AUTOMATICO;

uint32_t val_angulo[] = {550, 1037, 1524, 2011, 2500};
char cambio_angulo;
uint32_t veces50ms_barrido;
char haciendo_barrido;
uint8_t pos_angulo_barrido;


char fin_medio_segundo;
uint32_t veces50ms;
uint32_t veces_anti_rebotes;

uint32_t valor_adc;


enum estados{EMITIENDO, LEYENDO};
char estado, estado_anterior;
unsigned int delay, delay_previo;
char recibiendo;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM4_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

void habilitar_boton(unsigned int n);
void inhabilitar_boton(unsigned int n);
void arrancar_crono_antirebotes();
void parar_crono_antirebotes();

void encender_led_MODO();
void apagar_led_MODO();

void encender_led_MEDIDA();
void apagar_led_MEDIDA();

void encender_led_distancia(char i);
void apagar_led_distancia(char i);

void encender_N_leds(char n);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


/*
 * Handler del TIM4
 * */
void TIM4_IRQHandler(void){

  if(((TIM4 -> SR) & (1<<1)) != 0){
    veces50ms++;

    /*
     * 10 -> 10*50ms = 500ms = 0.5s
     * 100 -> 5s
     * */
    if(veces50ms == 10)
      fin_medio_segundo = 1;

    TIM4->SR &= ~(1<<1); //LIMPIAR EL FLAG
  }
  else{
    if(((TIM4 -> SR) & (1<<2)) != 0){
      if(estado == EMITIENDO){
        estado = LEYENDO;
        TIM4->CCR2 += 38000;
      }
      else{
        estado = EMITIENDO;
        TIM4->CCR2 += 10;   //10 us
      }
      TIM4->SR &= ~(1<<2); //LIMPIAR EL FLAG
    }
    else{
        if(((TIM4 -> SR) & (1<<3)) != 0){
          veces_anti_rebotes++;
          if(veces_anti_rebotes == VECES_REBOTES){
            parar_crono_antirebotes();
            habilitar_boton(13);
            habilitar_boton(4);
          }
          TIM4->SR &= ~(1<<3); //LIMPIAR EL FLAG
        }
        else{ // CH4
          veces50ms_barrido++;
          if(veces50ms_barrido == 40){//han pasado dos segundos
            cambio_angulo = 1;
            veces50ms_barrido = 0;
          }
          TIM4->CCR4 += 50000;
          TIM4->SR &= ~(1<<4); //LIMPIAR EL FLAG

        }
      }
    }
  }




////////////////////////////////////////////////////////////////

/*
 * Para medir el echo utilizamos el TIM3_CH4, el del PC9.
 * */
////////////////////////////////////////////////////////////////

void TIM3_IRQHandler(void){
  if( (TIM3 -> SR & (1<<4)) != 0){
  if(recibiendo == 0){
    recibiendo = 1;
    TIM3->CCER = 0x3000;//falling CH4 + CC4E a 1
    delay_previo = TIM3->CCR4;
  }
  else{
    recibiendo = 0;
    TIM3->CCER = 0x1000;//rising CH4 + CC4E a 1
    delay = TIM3->CCR4-delay_previo;

  }

  TIM3->SR &= ~(1<<4);
  }
}


/*
 * HANDLER ADC
 * */
void ADC1_IRQHandler(void){
  if((ADC1 -> SR & (1<<1)) != 0){ // HA SALTADO POR EL EOC
    valor_adc = ADC1->DR;
  }
}



/*
 * HANDLER INICIO DE MEDIDA
 * */
void EXTI4_IRQHandler(void){
  if((EXTI->PR & (1<<4) )!= 0){
    hacer_medida = 1;
    inhabilitar_boton(4);
    arrancar_crono_antirebotes();
    EXTI -> PR |= (1<<4); //limpiar el flag
  }

}




/*
 * HANDLER CAMBIO DE MODO
 * */
void EXTI15_10_IRQHandler(void){

  if((EXTI->PR & (1<<13) )!= 0){
    cambio_modo = 1;
    inhabilitar_boton(13);
    arrancar_crono_antirebotes();
    EXTI -> PR |= (1<<13); //limpiar el flag
  }

}


void habilitar_boton(unsigned int n){
  EXTI->IMR |= 1 <<n;          //HABILITA la exti
}


void inhabilitar_boton(unsigned int n){
  EXTI->IMR &= ~(1 <<n);          //HABILITA la exti
}

/*
 * TIM4_CH3 para el crono antirebotes
 * */
void arrancar_crono_antirebotes(){
  veces_anti_rebotes = 0;
  TIM4->CCR3 = TIM4->CNT + 100; //avísame dentro de 100 us.
  TIM4->DIER |= (1<<3); // INT del CH3 on
}

void parar_crono_antirebotes(){
  TIM4->DIER &= ~(1<<3); // INT del CH3 off
}


////////////////////////////////////////
//el TRIGGER delantero está en PC8
////////////////////////////////////////
void trigger_ON(){
  GPIOC->BSRR |= (1<<8);
}

void trigger_OFF(){
  GPIOC->BSRR |= (1<<(8+16));
}




//////////////////////////////
//El PA5 es el led de modo
//////////////////////////////
void encender_led_MODO(){
  GPIOA -> BSRR |= (1<<(5));
}

void apagar_led_MODO(){
  GPIOA -> BSRR |= (1<<(5+16));
}



//////////////////////////////
//El PB5 es el led de modo
//////////////////////////////
void encender_led_MEDIDA(){
  GPIOB -> BSRR |= (1<<(5));
}

void apagar_led_MEDIDA(){
  GPIOB -> BSRR |= (1<<(5+16));
}


//////////////////////////////
//El PC0-PC4 LEDs de distancia
//////////////////////////////
void encender_led_distancia(char i){
  GPIOC -> BSRR |= (1<<(i));
}

void apagar_led_distancia(char i){
  GPIOC -> BSRR |= (1<<(i+16));
}

void encender_N_leds(char n){
  for(char i = 0; i<n; i++)
    encender_led_distancia(i);
  for(char i = n; i<5; i++)
    apagar_led_distancia(i);
}


void set_DC(uint32_t valor){
  TIM2->CCR1 = valor;
}

void arrancar_TIM_2secs(){
  TIM4->CCR4 = TIM4->CNT + 50000; //avísame dentro de 50 ms.
  TIM4->DIER |= (1<<4); // INT del CH4 on
}

void parar_TIM_2secs(){
  TIM4->DIER &= ~(1<<4); // INT del CH4 off
}


void reset_barrido(){
  parar_TIM_2secs();
  haciendo_barrido = 0;
  pos_angulo_barrido = 0;
  cambio_angulo = 0;
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */


  //////////////////////////////////////////////////////  Configuro PA0 función alternativa TIM2 PWM: servomotor

   GPIOA->MODER |=  1<<1;
   GPIOA->MODER &= ~1;
   GPIOA->AFR[0] = 0x00000001;


  //////////////////////////////////////////////////////  Configuro PC8 digital output ECHO
  GPIOC -> MODER &= ~(1 << (8*2 + 1));  // coloca un 0
  GPIOC -> MODER |=  (1 << 8*2);      // coloca un 1

  ////////////////////////////////////////////////////////////////
  //PC9 TIM3 para el echo, modo AF
  ////////////////////////////////////////////////////////////////
  GPIOC->MODER |=  (1<<(9*2+1));  // coloca 1
  GPIOC->MODER &= ~(1<<(9*2));  // coloca 0

  GPIOC->AFR[1] &=  ~(1<<(1*4+3)); //Selección del TIM3 0010
  GPIOC->AFR[1] &=  ~(1<<(1*4+2));
  GPIOC->AFR[1] |=   (1<<(1*4+1));
  GPIOC->AFR[1] &=  ~(1<<(1*4));

  //////////////////////////////////////////////////////  Configuro PA5 y PB5: OUTPUT DIGITAL
  //////////////////////////////////////////////////////  PC0 - PC4: OUTPUT DIGITAL LEDs

  GPIOA -> MODER &= ~(1 << (5*2 + 1));  // coloca un 0
  GPIOA -> MODER |=  (1 << 5*2);     // coloca un 1

  GPIOB -> MODER &= ~(1 << (5*2 + 1));  // coloca un 0
  GPIOB -> MODER |=  (1 << 5*2);     // coloca un 1

  for(char i = 0; i<5; i++){ //PC0 al PC4
    GPIOC -> MODER &= ~(1 << (i*2 + 1));  // coloca un 0
    GPIOC -> MODER |=  (1 << i*2);     // coloca un 1
  }


  //////////////////////////////////////////////////////  Configuro PC13 y PB4: INPUT DIGITAL
  //////////////////////////////////////////////////////
  GPIOC -> MODER &= ~(1 << (13*2 + 1));  // coloca un 0
  GPIOC -> MODER &= ~(1 << (13*2));  // coloca un 0
  GPIOB -> MODER &= ~(1 << (4*2 + 1));  // coloca un 0
  GPIOB -> MODER &= ~(1 << (4*2));  // coloca un 0

  //FLOTANTE PC13
  GPIOC -> PUPDR &= ~(1 << (13*2+1));  // coloca un 0
  GPIOC -> PUPDR &= ~(1 << (13*2));  // coloca un 0
  //PULL UP PB4
  GPIOB -> PUPDR &= ~(1 << (4*2+1));  // coloca un 0
  GPIOB -> PUPDR |= (1 << (4*2));  // coloca un 1

  //EXTI PC13
  SYSCFG->EXTICR[3] = 2 << 4;   //EXTI-13 al Puerto C
  habilitar_boton(13);//EXTI->IMR |= 1 <<13;          //HABILITA la exti
  EXTI->RTSR &= ~(1 <<13);         //NO EN RISING
  EXTI->FTSR |= 1 <<13;         //EN FALLING
  //EXTI PB4
  SYSCFG->EXTICR[1] = 1 << (4*0);   //EXTI-4 al Puerto B
  habilitar_boton(4); //EXTI->IMR |= 1 <<4;          //HABILITA la exti
  EXTI->RTSR &= ~(1 <<4);         //NO EN RISING
  EXTI->FTSR |= 1 <<4;         //EN FALLING
  //NVIC
  NVIC -> ISER[1] |= (1<<8);    //habilita la fuente 40
  NVIC -> ISER[0] |= (1<<10);    //habilita la fuente 10

  //////////////////////////////////////////////////////



  //////////////////////////////////////////////////////  Configuro PA1: ANALÓGICO

  GPIOA -> MODER |=  (1 << (2*1+1));  // coloca un
  GPIOA -> MODER |=  (1 <<(2*1));     // coloca un 1

  //Configuro el ADC
  ADC1->CR2 = 0;

  ADC1->CR1 = 0x00000020;           // res de 12, sin scan, interrup ON
  ADC1->CR2 = 0x0000412;            // continuo

  ADC1->SQR1 = 0;                   //1 CANAL
  ADC1->SQR5 = 1;                   //ADC_IN1 para el PA1

  ADC1->CR2 |= 1;                   //ON
  while((ADC1->SR & 0x0040) == 0);  //espera activa
  ADC1->CR2 |= 1 << 30;             //START

  NVIC->ISER[0] |= 1<<18;



  //////////////////////////////////////////////////////  Configuro TIM4_CH1 para el medio segundo.

  TIM4->CR1 = 0;
  TIM4->CR2 = 0;
  TIM4->SMCR = 0;

  //TIM4->CNT = 0;
  TIM4->PSC = 31;      //1 step = 1us
  TIM4->ARR = 0xFFFF;
  TIM4->CCR1 = 50000;       //CH1 //50ms
  TIM4->CCR2 = 10;       //CH2 10us
 // TIM4->CCR4 = 50000;       //CH2 10us

  TIM4->DIER |= (1<<1); //interrupción CH1
  TIM4->DIER |= (1<<2); //interrupción CH2
  //EL CH3 también se usa, pero se está dedicado al antirebotes y se gestiona con unas funciones que se pueden ver arriba.
 // TIM4->DIER |= (1<<4); //interrupción CH4

  NVIC->ISER[0] |= (1<<30);

  TIM4->CCMR1 = 0;
  TIM4->CCER = 0;

  TIM4->CR1 |= (1<<0);
  TIM4->EGR = 0;
  TIM4->SR = 0;


  ////////////////////////////////////////////////////////////////
  //TIM3CH4 para el echo
  ////////////////////////////////////////////////////////////////
  TIM3->CR1 = 0;
  TIM3->CR2 = 0;
  TIM3->SMCR = 0;

  //con 32 -> 1000000 pasos/segundo (cada paso es 1 us)
  TIM3->PSC = 32-1;
  TIM3->CNT = 0; //contador a 0
  TIM3->ARR = 0xffff; //el tope de cuenta
  //EL CCRy en TIC no se escribe, se consulta

  //interrupción
  TIM3->DIER |= (1<<4); // interrupción del CH4

  TIM3->CCMR2 = 0x0100;
  TIM3->CCER = 0x1000;//rising (00) + CC4E a 1

  //inicializo el timer
  TIM3->CR1 |= 0x0001; //ON
  TIM3->EGR |= 0x0001; //UG para actualización
  TIM3->SR  = 0; //flags bajados

  NVIC->ISER[0] |= (1 << 29); //TIM3

  ////////////////////////////////////////////////////////////////////////////////
   // CONFIGURACIÓN TIM2_CH1 en PWM
   ////////////////////////////////////////////////////////////////////////////////
  TIM2->CR1 = 0;
    TIM2->CR2 = 0;
    TIM2->SMCR = 0;

    //conf. de la propia PWM
    TIM2->CNT = 0;
    TIM2->PSC = 31;
    TIM2->ARR = 19999;
    // 550 - 2500
    TIM2->CCR1 = 550; //<----------------------------------se cambia a mano aquí el DC

    // no nos hace falta la interrupción
   /* TIM2->DCR = 0;
    TIM2->DIER = 0x0000;
*/
    TIM2->CCMR1 = 0;
    TIM2->CCMR1 |= (1<<3);  //PE
    //Los 3 siguientes para PWM (ver manual)
    TIM2->CCMR1 |= (1<<6);  //1
    TIM2->CCMR1 |= (1<<5);  //1
    TIM2->CCMR1 &= ~(1<<4); //0

    TIM2->CCER |= (1<<0);   //CC1E
    TIM2->CR1 |= (1<<7);    //HW (bit ARPE)
    TIM2->EGR |= (1<<0);    //UG
    TIM2->CR1 |= (1<<0);    //ON
    TIM2->SR = 0;       //FLAG





  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */


  /////////////////////////////////////////// modo inicial AUTOMÁTICO
  encender_led_MODO();
  cambio_modo = 0;
  printf("Arranque por defecto: automatico\n");

  ///////////////////////////////////////////
  hacer_medida = 0;                         //flag para hacer la medida

  ///////////////////////////////////////////
  valor_adc = 0;                            //valor en decimal del ADC
  uint16_t posicion = 0;                    //posición actual en los diferentes rangos
  uint16_t posicion_previa = -1;
  uint16_t step = 0x0FFF / NUMERO_SECCIONES ; // resolución de 12 bits, el valor max es 0x0FFF
  printf("Step: %d\n", step);
  //int16_t angulo = 0;

  //char vez = 0; //auxiliar para el printf del ángulo
  ///////////////////////////////////////////
  haciendo_barrido = 0;
  pos_angulo_barrido = 0;
  cambio_angulo = 0;
  veces50ms_barrido = 0;

  ///////////////////////////////////////////
  fin_medio_segundo = 0;
  veces50ms = 0;

  ///////////////////////////////////////////

  recibiendo = 0;
  delay = 0;
  delay_previo = 0;

  estado_anterior = -1;
  estado = EMITIENDO;

  while (1)
  {

    // MÁQUINA DE ESTADOS DEL CAMBIO DE MODO
    if(cambio_modo == 1){
      cambio_modo = 0;
      if(modo == AUTOMATICO){
        modo = MANUAL;
        apagar_led_MODO();
        reset_barrido();
      //  printf("modo MANUAL\n");

      }
      else{
        modo = AUTOMATICO;
        encender_led_MODO();
      //  printf("modo AUTOMATICO\n");
      }
    }

    // MÁQUINA DE ESTADOS DE LA TOMA DE MEDIDA
    if(hacer_medida == 1){
       hacer_medida = 0;
       if(modo == AUTOMATICO){
          // printf("rearranque del barrido\n");
           haciendo_barrido = 1;
           pos_angulo_barrido = 0;
           cambio_angulo = 1;
           arrancar_TIM_2secs();
         }
       else{ //MANUAL
         encender_led_MEDIDA();
         TIM4->CNT = 0; //reinicio de cuenta
         TIM4->CCR1 = 50000;       //CH1 //50ms
         veces50ms = 0;
         TIM4->SR = 0;
         //TIM4->CR1 |= (1<<0); //ARRANCA el crono
        // printf("midiendo distancia\n");

       }
     }

    //MÁQUINA DE ESTADOS para las medidas cada 2 segundos
    if(haciendo_barrido == 1 && cambio_angulo == 1){
      cambio_angulo = 0;
      set_DC(val_angulo[pos_angulo_barrido++]);
      if(pos_angulo_barrido == 5){
        reset_barrido();
      }
    }



    // MÁQUINA DE ESTADOS DE LECTURA DEL ADC

       if(modo == MANUAL){
         posicion = valor_adc / step - 1; //get_posicion(valores, valor_adc);
          if(posicion != posicion_previa){
            posicion_previa = posicion;
           // angulo = (posicion - 9)*10;
            //comentado para que no moleste
            //printf("[%d] valor_adc: %ld, posicion: %d angulo: %d\n", vez++, valor_adc, posicion, angulo);
            set_DC(posicion*102+550);
          }
       }


    //MÁQUINA DE ESTADOS DEL TIMER PARA MEDIO SEGUNDO:
    if(modo == MANUAL){
      if(fin_medio_segundo == 1){
        fin_medio_segundo = 0;
        apagar_led_MEDIDA();
       // TIM4->CR1 &= ~0x01; //OFF TIMER
      }
    }


    //MÁQUINA DE ESTADOS DEL SENSOR:
    if(estado_anterior != estado){
       estado_anterior = estado;
       switch(estado){
         case EMITIENDO:
           trigger_ON();
           break;
         case LEYENDO:
           trigger_OFF();
           break;
       }
     }

    // d = t·0.034/2 -> t = 2d/0.034
    //                  si d = 80 cm -> t = 4705.88
    //                  si d = 70 cm -> t = 4117.65
    //                  si d = 60 cm -> t = 3529.41
    //                  si d = 30 cm -> t = 1764.7
    //                  si d = 10 cm -> t = 588.23
    if(modo == MANUAL){

      if(delay > 2353) //40cm
        encender_N_leds(0);
      else
        if(delay > 2059) //35cm
          encender_N_leds(1);
        else
          if(delay > 1765) //30cm
            encender_N_leds(2);
          else
              if(delay > 1176) //20cm
                encender_N_leds(3);
              else
                  if(delay > 589) // 10cm
                    encender_N_leds(4);
                  else
                    encender_N_leds(5);
    }
    else{
      encender_N_leds(0);
    }


    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLL_DIV3;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC_Init(void)
{

  /* USER CODE BEGIN ADC_Init 0 */

  /* USER CODE END ADC_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC_Init 1 */

  /* USER CODE END ADC_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc.Instance = ADC1;
  hadc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hadc.Init.LowPowerAutoWait = ADC_AUTOWAIT_DISABLE;
  hadc.Init.LowPowerAutoPowerOff = ADC_AUTOPOWEROFF_DISABLE;
  hadc.Init.ChannelsBank = ADC_CHANNELS_BANK_A;
  hadc.Init.ContinuousConvMode = DISABLE;
  hadc.Init.NbrOfConversion = 1;
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.DMAContinuousRequests = DISABLE;
  if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_4CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC_Init 2 */

  /* USER CODE END ADC_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 0;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 0;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 65535;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */



__attribute__((weak)) int _write(int file, char *ptr, int len)
{
  (void)file;
  int DataIdx;

  for (DataIdx = 0; DataIdx < len; DataIdx++)
  {
    ITM_SendChar(*ptr++);
  }
  return len;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
