#include "./SYSTEM/sys/sys.h"

/**
 * @brief       判断I_Cache是否打开
 * @param       无
 * @retval      返回值:0 关闭；1 打开
 */
uint8_t get_icahce_sta(void)
{
    uint8_t sta;
    sta = ((SCB->CCR)>>17) & 0X01;
    return sta;
}

/**
 * @brief       判断D_Cache是否打开
 * @param       无
 * @retval      返回值:0 关闭；1 打开
 */
uint8_t get_dcahce_sta(void)
{
    uint8_t sta;
    sta = ((SCB->CCR)>>16) & 0X01;
    return sta;
}

/**
 * @brief       设置中断向量表偏移地址
 * @param       baseaddr: 基地址
 * @param       offset: 偏移量
 * @retval      无
 */
void sys_nvic_set_vector_table(uint32_t baseaddr, uint32_t offset)
{
    /* 设置NVIC的中断向量表偏移寄存器,VTOR低9位保留,即[8:0]保留 */
    SCB->VTOR = baseaddr | (offset & (uint32_t)0xFFFFFE00);
}

/**
 * @brief       执行: WFI指令(执行该指令进入低功耗状态, 等待中断唤醒)
 * @param       无
 * @retval      无
 */
void sys_wfi_set(void)
{
    __ASM volatile("wfi");
}

/**
 * @brief       关闭所有中断(但是不包括fault和NMI中断)
 * @param       无
 * @retval      无
 */
void sys_intx_disable(void)
{
    __ASM volatile("cpsid i");
}

/**
 * @brief       开启所有中断
 * @param       无
 * @retval      无
 */
void sys_intx_enable(void)
{
    __ASM volatile("cpsie i");
}

/**
 * @brief       设置栈顶地址
 * @note        左汇编, 在MDK中, 实际上没有用到
 * @param       addr: 栈顶地址
 * @retval      无
 */
void sys_msr_msp(uint32_t addr)
{
    __set_MSP(addr);        /* 设置栈顶地址 */
}

/**
 * @brief       使能STM32H7的L1-Cache, 同时开启D cache的强制透写
 * @param       无
 * @retval      无
 */
void sys_cache_enable(void)
{
    SCB_EnableICache();     /* 使能I-Cache,此函数在core_cm7.h中定义 */
    SCB_EnableDCache();     /* 使能D-Cache,此函数在core_cm7.h中定义 */
    SCB->CACR |= 1 << 2;    /* 强制D-Cache透写,若不开启透写,在使用DMA时可能导致数据不一致 */
}

/**
 * @brief       系统时钟初始化函数
 * @param       plln: PLL1倍频系数(PLL倍频), 取值范围: 4~512.
 * @param       pllm: PLL1预分频系数(进PLL之前的分频), 取值范围: 2~63.
 * @param       pllp: PLL1的p分频系数(PLL之后的分频), 分频后作为系统时钟, 取值范围: 2~128.(且必须是2的倍数)
 * @param       pllq: PLL1的q分频系数(PLL之后的分频), 取值范围: 1~128.
 * @note
 *
 *              Fvco: VCO频率
 *              Fsys: 系统时钟频率, 也是PLL1的p分频输出时钟频率
 *              Fq:   PLL1的q分频输出时钟频率
 *              Fs:   PLL输入时钟频率, 可以是HSI, CSI, HSE等.
 *              Fvco = Fs * (plln / pllm);
 *              Fsys = Fvco / pllp = Fs * (plln / (pllm * pllp));
 *              Fq   = Fvco / pllq = Fs * (plln / (pllm * pllq));
 *
 *              外部晶振为25M的时候, 推荐值: plln = 160, pllm = 5, pllp = 2, pllq = 4.
 *              得到:Fvco = 25 * (160 / 5) = 800Mhz
 *                   Fsys = pll1_p_ck = 800 / 2 = 400Mhz
 *                   Fq   = pll1_q_ck = 800 / 4 = 200Mhz
 *
 *              H743默认需要配置的频率如下:
 *              CPU频率(rcc_c_ck) = sys_d1cpre_ck = 400Mhz
 *              rcc_aclk = rcc_hclk3 = 200Mhz
 *              AHB1/2/3/4(rcc_hclk1/2/3/4) = 200Mhz
 *              APB1/2/3/4(rcc_pclk1/2/3/4) = 100Mhz
 *              pll2_p_ck = (25 / 25) * 440 / 2) = 220Mhz
 *              pll2_r_ck = FMC时钟频率 = ((25 / 25) * 440 / 2) = 220Mhz
 *
 * @retval      执行结果: 0, 成功; 1, 失败;
 */
uint8_t sys_stm32_clock_init(uint32_t plln, uint32_t pllm, uint32_t pllp, uint32_t pllq)
{
    HAL_StatusTypeDef ret = HAL_OK;
    RCC_ClkInitTypeDef rcc_clk_init_handle;
    RCC_OscInitTypeDef rcc_osc_init_handle;

    MODIFY_REG(PWR->CR3, PWR_CR3_SCUEN, 0);                         /* 使能过驱动配置 */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);  /* VOS = 1, Scale1, 1.2V内核电压,FLASH访问速度可以得到最大提高 */
    while ((PWR->D3CR & (PWR_D3CR_VOSRDY)) != PWR_D3CR_VOSRDY);     /* 等待电压稳定 */

    /* 使能HSE，并选择HSE作为PLL时钟源，并配置PLL1，同时使能USB时钟 */
    rcc_osc_init_handle.OscillatorType = RCC_OSCILLATORTYPE_HSE | RCC_OSCILLATORTYPE_HSI48;
    rcc_osc_init_handle.HSEState = RCC_HSE_ON;
    rcc_osc_init_handle.HSIState = RCC_HSI_OFF;
    rcc_osc_init_handle.CSIState = RCC_CSI_OFF;
    rcc_osc_init_handle.HSI48State = RCC_HSI48_ON;
    rcc_osc_init_handle.PLL.PLLState = RCC_PLL_ON;
    rcc_osc_init_handle.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    rcc_osc_init_handle.PLL.PLLN = plln;
    rcc_osc_init_handle.PLL.PLLM = pllm;
    rcc_osc_init_handle.PLL.PLLP = pllp;
    rcc_osc_init_handle.PLL.PLLQ = pllq;
    rcc_osc_init_handle.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    rcc_osc_init_handle.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
    rcc_osc_init_handle.PLL.PLLFRACN = 0;
    ret = HAL_RCC_OscConfig(&rcc_osc_init_handle);
    if (ret != HAL_OK)
    {
        return 1;
    }

    /*
     *  选择PLL输出作为系统时钟
     *  配置RCC_CLOCKTYPE_SYSCLK系统时钟,400M
     *  配置RCC_CLOCKTYPE_HCLK 时钟,200Mhz,对应AHB1，AHB2，AHB3，AHB4总线
     *  配置RCC_CLOCKTYPE_PCLK1时钟,100Mhz,对应APB1总线
     *  配置RCC_CLOCKTYPE_PCLK2时钟,100Mhz,对应APB2总线
     *  配置RCC_CLOCKTYPE_D1PCLK1时钟,100Mhz,对应APB3总线
     *  配置RCC_CLOCKTYPE_D3PCLK1时钟,100Mhz,对应APB4总线
     */
    rcc_clk_init_handle.ClockType = (RCC_CLOCKTYPE_SYSCLK \
                                    | RCC_CLOCKTYPE_HCLK \
                                    | RCC_CLOCKTYPE_PCLK1 \
                                    | RCC_CLOCKTYPE_PCLK2 \
                                    | RCC_CLOCKTYPE_D1PCLK1 \
                                    | RCC_CLOCKTYPE_D3PCLK1);

    rcc_clk_init_handle.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    rcc_clk_init_handle.SYSCLKDivider = RCC_SYSCLK_DIV1;
    rcc_clk_init_handle.AHBCLKDivider = RCC_HCLK_DIV2;
    rcc_clk_init_handle.APB1CLKDivider = RCC_APB1_DIV2; 
    rcc_clk_init_handle.APB2CLKDivider = RCC_APB2_DIV2; 
    rcc_clk_init_handle.APB3CLKDivider = RCC_APB3_DIV2;  
    rcc_clk_init_handle.APB4CLKDivider = RCC_APB4_DIV2; 
    ret = HAL_RCC_ClockConfig(&rcc_clk_init_handle, FLASH_LATENCY_2);
    if (ret != HAL_OK)
    {
        return 1;
    }

    HAL_PWREx_EnableUSBVoltageDetector();   /* 使能USB电压探测 */
    __HAL_RCC_CSI_ENABLE() ;                /* 使能CSI时钟 */
    __HAL_RCC_SYSCFG_CLK_ENABLE() ;         /* 使能SYSCFG时钟 */
    HAL_EnableCompensationCell();           /* 使能IO补偿单元 */
    return 0;
}

#ifdef  USE_FULL_ASSERT

/**
 * @brief       当出错时报告出错的文件名和行号
 * @param       file：指向源文件名的指针
 * @param       line：指定文件中的出错行号
 * @retval      无
 */
void assert_failed(uint8_t *file, uint32_t line)
{ 
    while (1)
    {
    }
}
#endif
