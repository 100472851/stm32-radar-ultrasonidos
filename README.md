# Radar de ultrasonidos con STM32L152

Sistema embebido sobre la placa **STM32L152** (Nucleo/Discovery) que mide distancias con un sensor de ultrasonidos montado sobre un servomotor. Proyecto de *Sistemas Digitales Basados en Microprocesadores* (UC3M).

## Funcionamiento
- **Modo automático:** el servo barre 5 posiciones, cambiando de ángulo cada 2 s, y en cada una se toma una medida de distancia.
- **Modo manual:** el ángulo se fija con un potenciómetro leído por el **ADC** (19 secciones).
- La distancia se muestra en una barra de LED; un botón cambia de modo y otro lanza la medida, con antirrebotes por temporizador.

## Periféricos
| Periférico | Uso |
|---|---|
| TIM2 (PWM) | Posición del servo (pulsos de 550 a 2500 µs) |
| TIM3 CH4 (captura de entrada) | Medida del tiempo de eco del sensor, capturando flanco de subida y de bajada |
| TIM4 (4 canales de comparación) | CH1: bases de 50 ms y 0,5 s · CH2: pulso de disparo de 10 µs · CH3: antirrebotes · CH4: cambio de ángulo del barrido cada 2 s |
| ADC (interrupción EOC) | Lectura del potenciómetro en modo manual |
| EXTI4 / EXTI13 | Botón de inicio de medida y botón de cambio de modo |
| USART2 | Trazas de depuración |

La lógica está organizada como varias **máquinas de estados** en el bucle principal (cambio de modo, toma de medida, barrido, lectura del ADC, sensor), alimentadas por interrupciones.

## Ficheros
- [`src/main.c`](src/main.c): programa principal. Mi código está entre los bloques `USER CODE`; el resto lo genera STM32CubeMX.
- [`docs/`](docs/): diagramas de bloques y de flujo.

Para compilarlo hay que crear un proyecto en STM32CubeIDE para la STM32L152RE con TIM2, TIM3, TIM4, ADC y USART2, y sustituir `main.c`.
