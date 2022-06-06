/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
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
#include "cmsis_os.h"
#include "dma.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "filesys_api.h"
#include "sensor_board.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t xband_buf[XBAND_BUF_SIZE];
uint8_t sbc_buf[SBC_BUF_SIZE]; //sensor_board/camera buffer

QueueHandle_t xband_queue, camera_queue, device_status_queue, sensor_board_queue;
TaskHandle_t InterpTask_handle;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void vTaskXBand(void *pvParameters){
	uint16_t file, current_pack_size;
	uint32_t file_size;
	struct xband_params xband;
	struct pdh_device_status pdh_device;
	pdh_device.device = dev_xband;

	while(1){
		pdh_device.status = PDH_DEVICE_OK;
		pdh_device.errno = 0xFFFFFFFF;
		pdh_device.target_file_name = 0xFFFF;

		xQueueReceive(xband_queue, &xband, portMAX_DELAY);

		if( xband.reinit_filesys ){
			if( reinit_fs()==FS_FAIL ){
				pdh_device.status = PDH_DEVICE_ERR;
				pdh_device.target_file_name = 0xFFFF;
				pdh_device.errno = errno;
			}
		} else {
			if(xband.filename_to_transmit!=0xFFFF){
				file = open(xband.filename_to_transmit, O_RDONLY);
				if( file==0xFFFF ){
					pdh_device.status = PDH_DEVICE_ERR;
					pdh_device.target_file_name = xband.filename_to_transmit;
					pdh_device.errno = errno;
					xband.delete_transmited_yn = 0;
					xband.filename_to_delete = 0xFFFF;
					file_size = 0;
				} else {
					file_size = get_file_size(file);
				}

				if( file_size==0xFFFFFFFF ){
					pdh_device.status = PDH_DEVICE_ERR;
					pdh_device.target_file_name = xband.filename_to_transmit;
					pdh_device.errno = errno;
					xband.delete_transmited_yn = 0;
					xband.filename_to_delete = 0xFFFF;
					file_size = 0;
				}

				while( file_size>0 ){
					if( file_size>XBAND_BUF_SIZE ){
						current_pack_size = XBAND_BUF_SIZE;
						file_size -= XBAND_BUF_SIZE;
					} else {
						current_pack_size = file_size;
						file_size = 0;
					}

					if( read(file, xband_buf, current_pack_size)==0xFFFFFFFF ){
						pdh_device.status = PDH_DEVICE_ERR;
						pdh_device.target_file_name = xband.filename_to_transmit;
						pdh_device.errno = errno;
						xband.delete_transmited_yn = 0;
						xband.filename_to_delete = 0xFFFF;
						break;
					}

					uart_send_data(xband_buf,current_pack_size );
				}

				if(xband.delete_transmited_yn){
					if( remove(xband.filename_to_transmit)==0xFF){
						pdh_device.status = PDH_DEVICE_ERR;
						pdh_device.target_file_name = xband.filename_to_transmit;
						pdh_device.errno = errno;
						xband.filename_to_delete = 0xFFFF;
					}
				}
			}
			if(xband.filename_to_delete!=0xFFFF){
				if( remove(xband.filename_to_delete)==0xFF){
					pdh_device.status = PDH_DEVICE_ERR;
					pdh_device.target_file_name = xband.filename_to_delete;
					pdh_device.errno = errno;
				}
			}
		}
		xQueueSendToBack(device_status_queue, &pdh_device, 0);
	}
}

void vTaskCamera(void *pvParameters){
	uint16_t file, current_pack_size;
	uint32_t img_size;
	struct camera_params camera;
	struct pdh_device_status pdh_device;
	pdh_device.device = dev_camera;

	while(1){
		pdh_device.status = PDH_DEVICE_OK;
		pdh_device.errno = 0xFFFFFFFF;
		pdh_device.target_file_name = 0xFFFF;

		xQueueReceive(camera_queue, &camera, portMAX_DELAY);

		file = open(camera.file_name, O_CREAT|O_WRONLY|O_JWEAK);
		if(file==0xFFFF){
			pdh_device.status = PDH_DEVICE_ERR;
			pdh_device.target_file_name = camera.file_name;
			pdh_device.errno = errno;
		} else {
			ACAM_Reset();

			if( camera.format==PDH_IMG_FMT_JPG){
				ACAM_select_JPEG();
			} else {
				ACAM_select_RAW(1); //resolution 5 mp
			}

			ACAM_exp_gain_manual();
			ACAM_set_exposure(camera.exp_nr_lines,camera.exp_nr_lines_frac);
			ACAM_set_gain(camera.gain);

			ACAM_start_capture();
			while( !ACAM_is_cap_complete() );
			wait_for(500,TIM_UNIT_MS);

	    	img_size = ACAM_get_image_size();

	    	while( img_size > 0 ){
	    		if( img_size > SBC_BUF_SIZE ){
				  current_pack_size = SBC_BUF_SIZE;
				  img_size -= SBC_BUF_SIZE;
	    		} else {
				  current_pack_size = img_size;
				  img_size = 0;
	    		}

	    		ACAM_spi_read_package(sbc_buf, current_pack_size);

	    		if( write(file, sbc_buf, current_pack_size )==0xFFFFFFFF ){
	    			pdh_device.status = PDH_DEVICE_ERR;
	    			pdh_device.target_file_name = camera.file_name;
	    			pdh_device.errno = errno;
	    		}
	    	}
	    	pdh_device.target_file_name = camera.file_name;
		}
		xQueueSendToBack(device_status_queue, &pdh_device, 0);
	}
}

void vTaskSensor(void *pvParameters) {
	uint16_t file;
	struct sensor_params params;
	struct pdh_device_status pdh_device;
	pdh_device.device = dev_sensor_board;

	while(1) {
		pdh_device.status = PDH_DEVICE_OK;
		pdh_device.errno = 0xFFFFFFFF;
		pdh_device.target_file_name = 0xFFFF;

		xQueueReceive(sensor_board_queue, &params, portMAX_DELAY);

		Sensor_Board sb;
		SB_Init(&sb);
		SB_Get_Temperature_Readings(&sb);
		SB_Start_ADC_Sampling(&sb);
		SB_Get_Complex_Samples(&sb);

		file = open(params.file_name, O_CREAT|O_WRONLY|O_JWEAK);
		if(file==0xFFFF){
			pdh_device.status = PDH_DEVICE_ERR;
			pdh_device.target_file_name = params.file_name;
			pdh_device.errno = errno;
		} else {
			if( write(file, (void *) sb.adc->complex_samples, NUM_SAMPLES * 8 * 2 * 4)==0xFFFFFFFF ){
				pdh_device.status = PDH_DEVICE_ERR;
				pdh_device.target_file_name = params.file_name;
				pdh_device.errno = errno;
			}
			pdh_device.target_file_name = params.file_name;
		}

		xQueueSendToBack(device_status_queue, &pdh_device, 0);
	}
}

void vTaskInterpreter(void *pvParameters){
	struct pdh_params pdh;
	struct pdh_device_status pdh_dev_status;
	uint8_t s[8], collect_params = 0;
	uint8_t nr_sent, nr_rec;
	uint16_t u16_param;

	//default params
	pdh.camera.exp_nr_lines = 512;
	pdh.camera.exp_nr_lines_frac = 0;
	pdh.camera.gain = 1;
	pdh.camera.format = PDH_IMG_FMT_JPG;

	if(W25N_init()==0){
		printf_eig("w25n SPI fault\r");
		// <---------- MAKNUTI KOMENTARE
		// while(1);
	} else {
		printf_eig("startup_ok\r\n");
	}
	// <---------- MAKNUTI KOMENTARE
//	if( start_fs()==FS_FAIL ){
//		printf_eig("filesystem init failure.\r\n");
//	}


	while(1){
		//-----XBAND-BEGIN-----
				pdh.xband.reinit_filesys = 0;
				nr_sent = 0;
				nr_rec = 0;
				do {
					printf_eig("Type 'reinit' to reinitialize the filesystem, 'n' not to\n");
					if( gets_eig(s)!= NULL){
						if( !strcmp((char *)s,"reinit") ){
							pdh.xband.reinit_filesys = 1;
							break;
						} else if( !strcmp((char *)s,"n") ){
							break;
						} else {
							printf_eig("Invalid command!\n");
						}
					}
				} while(1);

				if(pdh.xband.reinit_filesys==1){
					xQueueSendToBack(xband_queue, &pdh.xband, 0);
					nr_sent++;
				} else {

					do {
						printf_eig("Press 'y' to select file for transmission, 'n' to skip\n");
						if( gets_eig(s)!= NULL){
							if( !strcmp((char *)s,"y") ){
								collect_params = 1;
								break;
							} else if( !strcmp((char *)s,"n") ){
								pdh.xband.filename_to_transmit = 0xFFFF;
								collect_params = 0;
								break;
							} else {
								printf_eig("Invalid command!\n");
							}
						}
					} while(1);

					if( collect_params ){
						collect_params = 0;
						do {
							printf_eig("Enter file name [0-1023] of file to be transmitted via x-band.\n");
							if( gets_eig(s)!= NULL){
								u16_param = parse_file_name(s);
								if( u16_param==0xFFFF ){
									printf_eig("File name must be a number between 0 and 1023 (included)\n");
								} else {
									pdh.xband.filename_to_transmit = u16_param;
									break;
								}
							}
						} while(1);

						do {
							printf_eig("Press 'y' to delete file after transmission, 'n' to keep it\n");
							if( gets_eig(s)!= NULL){
								if( !strcmp((char *)s,"y") ){
									pdh.xband.delete_transmited_yn = 1;
									break;
								} else if( !strcmp((char *)s,"n") ){
									pdh.xband.delete_transmited_yn = 0;
									break;
								} else {
									printf_eig("Invalid command!\n");
								}
							}
						} while(1);
					}

					do {
						printf_eig("Any [other] file you would like to delete?['y'/'n']\n");
						if( gets_eig(s)!= NULL){
							if( !strcmp((char *)s,"y") ){
								collect_params = 1;
								break;
							} else if( !strcmp((char *)s,"n") ){
								pdh.xband.filename_to_delete = 0xFFFF;
								collect_params =0;
								break;
							} else {
								printf_eig("Invalid command!\n");
							}
						}
					} while(1);

					if(collect_params){
						collect_params = 0;
						do {
							printf_eig("Enter file name [0-1023] of file to delete.\n");
							if( gets_eig(s)!= NULL){
								u16_param = parse_file_name(s);
								if( u16_param==0xFFFF ){
									printf_eig("File name must be a number between 0 and 1023 (included)\n");
								} else {
									pdh.xband.filename_to_delete = u16_param;
									break;
								}
							}
						} while(1);
					}
					xQueueSendToBack(xband_queue, &pdh.xband, 0);
					nr_sent++;

				}
				//-----XBAND-END-----
		//-----CAMERA-BEGIN-----
		do {
			printf_eig("Press 'y' to set camera params, 'd' to use default/previous, 'n' to skip\r\n");
			if( gets_eig(s)!= NULL){
				if( !strcmp((char *)s,"y") ){
					collect_params = 1;
					break;
				} else if( !strcmp((char *)s,"d") ){
					collect_params = 2;
					break;
				} else if( !strcmp((char *)s,"n") ){
					collect_params = 0;
					break;
				} else {
					printf_eig("Invalid command!\r\n");
				}
			}
		} while(1);

		if( collect_params==1 ){
			//collect new parameters
			collect_params = 0;

			do {
				printf_eig("Set exposure nr_lines (uint16_t).\r\n");
				if( gets_eig(s)!= NULL){
					pdh.camera.exp_nr_lines = parse_uint16(s);
					break;
				}
			} while(1);

			do {
				printf_eig("Set exposure nr_lines frac (uint8_t).\r\n");
				if( gets_eig(s)!= NULL){
					pdh.camera.exp_nr_lines_frac = (uint8_t)parse_uint16(s);
					break;
				}
			} while(1);

			do {
				printf_eig("Set gain (uint8_t), see acam.h.\r\n");
				if( gets_eig(s)!= NULL){
					pdh.camera.gain = (uint8_t)parse_uint16(s);
					break;
				}
			} while(1);

			do {
				printf_eig("Select format: 'r' for raw, 'j' for jpeg,\r\n");
				if( gets_eig(s)!= NULL){
					if( !strcmp((char *)s,"r") ){
						pdh.camera.format = PDH_IMG_FMT_RAW;
						break;
					} else if( !strcmp((char *)s,"j") ){
						pdh.camera.format = PDH_IMG_FMT_JPG;
						break;
					} else {
						printf_eig("Invalid command!\r\n");
					}
				}
			} while(1);

			do {
				printf_eig("Enter file name [0-1023] to store image measurments.\r\n");
				if( gets_eig(s)!= NULL){
					u16_param = parse_file_name(s);
					if( u16_param==0xFFFF ){
						printf_eig("File name must be a number between 0 and 1023 (included)\r\n");
					} else {
						pdh.camera.file_name = u16_param;
						break;
					}
				}
			} while(1);
			xQueueSendToBack(camera_queue, &pdh.camera, 0);
			nr_sent++;

		} else if(collect_params==2){
			//use defaults/previous value, only gather file name
			collect_params = 0;
			do {
				printf_eig("Enter file name [0-1023] to store image.\r\n");
				if( gets_eig(s)!= NULL){
					u16_param = parse_file_name(s);
					if( u16_param==0xFFFF ){
						printf_eig("File name must be a number between 0 and 1023 (included)\r\n");
					} else {
						pdh.camera.file_name = u16_param;
						break;
					}
				}
			} while(1);
			xQueueSendToBack(camera_queue, &pdh.camera, 0);
			nr_sent++;
		}
		//-----CAMERA-END-----

		//-----SENSOR-BOARD-START-----

		do {
			printf_eig("Press 'y' to set sensor params, 'n' to skip\r\n");
			if( gets_eig(s)!= NULL){
				if( !strcmp((char *)s,"y") ){
					collect_params = 1;
					break;
				} else if( !strcmp((char *)s,"n") ){
					collect_params = 0;
					break;
				} else {
					printf_eig("Invalid command!\r\n");
				}
			}
		} while(1);

		if( collect_params==1 ){
			//collect new parameters
			collect_params = 0;

			do {
				printf_eig("Enter file name [0-1023] to store sensor data.\r\n");
				if( gets_eig(s)!= NULL){
					u16_param = parse_file_name(s);
					if( u16_param==0xFFFF ){
						printf_eig("File name must be a number between 0 and 1023 (included)\r\n");
					} else {
						pdh.sensor_board.file_name = u16_param;
						break;
					}
				}
			} while(1);

			xQueueSendToBack(sensor_board_queue, &pdh.sensor_board, 0);
			nr_sent++;
		}

		//-----SENSOR-BOARD-END-------

		do {
			printf_eig("Press 's' to start PDH operations\r\n");
			if( gets_eig(s)!= NULL){
				if( !strcmp((char *)s,"s") ){
					pdh.xband.reinit_filesys = 1;
					break;
				} else {
					printf_eig("Invalid command!\r\n");
				}
			}
		} while(1);

		vTaskPrioritySet(NULL, 1); //drop interpreter task priority to enable execution of PDH device tasks

		do{
			xQueueReceive(device_status_queue, &pdh_dev_status, portMAX_DELAY);
			/*
			if (pdh_dev_status.device==dev_camera){
				printf_eig("Device camera status ");
			}
			*/

			if(pdh_dev_status.device==dev_xband){
				printf_eig("Device xband status ");
			} else if (pdh_dev_status.device==dev_camera){
				printf_eig("Device camera status ");
			}
			else {
				printf_eig("Device sensor board status ");
			}



			if( pdh_dev_status.status==PDH_DEVICE_OK ){
				printf_eig("ok\n");
			} else {
				printf_eig("err\n");
			}


			nr_rec++;
		} while(nr_rec!=nr_sent);

		vTaskPrioritySet(NULL, 4); //raise priority
	}
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
  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

  /* System interrupt init*/
  NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  /* PendSV_IRQn interrupt configuration */
  NVIC_SetPriority(PendSV_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),15, 0));
  /* SysTick_IRQn interrupt configuration */
  NVIC_SetPriority(SysTick_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),15, 0));

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_SPI1_Init();
  MX_DMA_Init();
  MX_SPI2_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  MX_SPI3_Init();
  /* USER CODE BEGIN 2 */
  // <---------- MAKNUTI KOMENTARE
//  ACAM_DMA_Enable();
//  ACAM_TestComms();

  xband_queue = xQueueCreate(1, sizeof(struct xband_params) );
  camera_queue = xQueueCreate(1, sizeof(struct camera_params) );
  sensor_board_queue = xQueueCreate(1, sizeof(struct sensor_params) );
  device_status_queue = xQueueCreate(3, sizeof(struct pdh_device_status) );

  xTaskCreate( vTaskInterpreter, "Interpreter Task", configMINIMAL_STACK_SIZE*2, NULL, 4, &InterpTask_handle);
  xTaskCreate( vTaskCamera, "Camera Task", configMINIMAL_STACK_SIZE*2, NULL, 2, NULL);
  xTaskCreate( vTaskXBand, "XBand & MemMang Task", configMINIMAL_STACK_SIZE*2, NULL, 3, NULL);
  xTaskCreate( vTaskSensor, "Sensor Board Task", configMINIMAL_STACK_SIZE*2, NULL, 1, NULL);

  vTaskStartScheduler();

  /* USER CODE END 2 */

  /* Call init function for freertos objects (in freertos.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_4);
  while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_4)
  {
  }
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
  LL_RCC_HSE_Enable();

   /* Wait till HSE is ready */
  while(LL_RCC_HSE_IsReady() != 1)
  {

  }
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_5, 32, LL_RCC_PLLR_DIV_2);
  LL_RCC_PLL_EnableDomain_SYS();
  LL_RCC_PLL_Enable();

   /* Wait till PLL is ready */
  while(LL_RCC_PLL_IsReady() != 1)
  {

  }
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {

  }
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_4);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);

  LL_Init1msTick(80000000);

  LL_SetSystemCoreClock(80000000);
}

/* USER CODE BEGIN 4 */

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

#ifdef  USE_FULL_ASSERT
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
