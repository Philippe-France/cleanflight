/*
 * This file is part of Cleanflight.
 *
 * Cleanflight is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Cleanflight is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Cleanflight.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "platform.h"

#include "build_config.h"

#include "common/axis.h"

#include "drivers/sensor.h"

#include "drivers/accgyro.h"
#include "drivers/accgyro_adxl345.h"
#include "drivers/accgyro_bma280.h"
#include "drivers/accgyro_l3g4200d.h"
#include "drivers/accgyro_mma845x.h"
#include "drivers/accgyro_mpu3050.h"
#include "drivers/accgyro_mpu6050.h"
#include "drivers/accgyro_l3gd20.h"
#include "drivers/accgyro_lsm303dlhc.h"

#include "drivers/accgyro_spi_mpu6000.h"
#include "drivers/accgyro_spi_mpu6500.h"

#include "drivers/barometer.h"
#include "drivers/barometer_bmp085.h"
#include "drivers/barometer_ms5611.h"

#include "drivers/compass.h"
#include "drivers/compass_hmc5883l.h"
#include "drivers/compass_ak8975.h"

#include "drivers/sonar_hcsr04.h"

#include "drivers/gpio.h"
#include "drivers/system.h"

#include "flight/flight.h"
#include "config/runtime_config.h"

#include "sensors/sensors.h"
#include "sensors/acceleration.h"
#include "sensors/barometer.h"
#include "sensors/gyro.h"
#include "sensors/compass.h"
#include "sensors/sonar.h"

#ifdef NAZE
#include "hardware_revision.h"
#endif

extern float magneticDeclination;

extern gyro_t gyro;
extern baro_t baro;
extern acc_t acc;

const mpu6050Config_t *selectMPU6050Config(void)
{
#ifdef NAZE
    // MPU_INT output on rev4/5 hardware (PB13, PC13)
    static const mpu6050Config_t nazeRev4MPU6050Config = {
            .gpioAPB2Peripherals = RCC_APB2Periph_GPIOB,
            .gpioPort = GPIOB,
            .gpioPin = Pin_13
    };
    static const mpu6050Config_t nazeRev5MPU6050Config = {
            .gpioAPB2Peripherals = RCC_APB2Periph_GPIOC,
            .gpioPort = GPIOC,
            .gpioPin = Pin_13
    };


    if (hardwareRevision < NAZE32_REV5) {
        return &nazeRev4MPU6050Config;
    } else {
        return &nazeRev5MPU6050Config;
    }
#endif

#ifdef SPRACINGF3
    static const mpu6050Config_t spRacingF3MPU6050Config = {
            .gpioAHBPeripherals = RCC_AHBPeriph_GPIOC,
            .gpioPort = GPIOC,
            .gpioPin = Pin_13
    };
    return &spRacingF3MPU6050Config;
#endif
    return NULL;
}

#ifdef USE_FAKE_GYRO
static void fakeGyroInit(void) {}
static void fakeGyroRead(int16_t *gyroData) {
    UNUSED(gyroData);
}
static void fakeGyroReadTemp(int16_t *tempData) {
    UNUSED(tempData);
}

bool fakeGyroDetect(gyro_t *gyro, uint16_t lpf)
{
    UNUSED(lpf);
    gyro->init = fakeGyroInit;
    gyro->read = fakeGyroRead;
    gyro->temperature = fakeGyroReadTemp;
    return true;
}
#endif

#ifdef USE_FAKE_ACC
static void fakeAccInit(void) {}
static void fakeAccRead(int16_t *accData) {
    UNUSED(accData);
}

bool fakeAccDetect(acc_t *acc)
{
    acc->init = fakeAccInit;
    acc->read = fakeAccRead;
    acc->revisionCode = 0;
    return true;
}
#endif

bool detectGyro(uint16_t gyroLpf)
{
    gyroAlign = ALIGN_DEFAULT;

#ifdef USE_GYRO_MPU6050
    if (mpu6050GyroDetect(selectMPU6050Config(), &gyro, gyroLpf)) {
#ifdef GYRO_MPU6050_ALIGN
        gyroAlign = GYRO_MPU6050_ALIGN;
#endif
        return true;
    }
#endif

#ifdef USE_GYRO_L3G4200D
    if (l3g4200dDetect(&gyro, gyroLpf)) {
#ifdef GYRO_L3G4200D_ALIGN
        gyroAlign = GYRO_L3G4200D_ALIGN;
#endif
        return true;
    }
#endif

#ifdef USE_GYRO_MPU3050
    if (mpu3050Detect(&gyro, gyroLpf)) {
#ifdef GYRO_MPU3050_ALIGN
        gyroAlign = GYRO_MPU3050_ALIGN;
#endif
        return true;
    }
#endif

#ifdef USE_GYRO_L3GD20
    if (l3gd20Detect(&gyro, gyroLpf)) {
#ifdef GYRO_GYRO_L3GD20_ALIGN
        gyroAlign = GYRO_GYRO_L3GD20_ALIGN;
#endif
        return true;
    }
#endif

#ifdef USE_GYRO_SPI_MPU6000
    if (mpu6000SpiGyroDetect(&gyro, gyroLpf)) {
#ifdef GYRO_SPI_MPU6000_ALIGN
        gyroAlign = GYRO_SPI_MPU6000_ALIGN;
#endif
        return true;
    }
#endif

#ifdef USE_GYRO_SPI_MPU6500
#ifdef NAZE
    if (hardwareRevision == NAZE32_SP && mpu6500SpiGyroDetect(&gyro, gyroLpf)) {
#ifdef GYRO_SPI_MPU6500_ALIGN
        gyroAlign = GYRO_SPI_MPU6500_ALIGN;
#endif
        return true;
    }
#else
    if (mpu6500SpiGyroDetect(&gyro, gyroLpf)) {
#ifdef GYRO_SPI_MPU6500_ALIGN
        gyroAlign = GYRO_SPI_MPU6500_ALIGN;
#endif
        return true;
    }
#endif
#endif

#ifdef USE_FAKE_GYRO
    if (fakeGyroDetect(&gyro, gyroLpf)) {
        return true;
    }
#endif
    return false;
}

static void detectAcc(uint8_t accHardwareToUse)
{
#ifdef USE_ACC_ADXL345
    drv_adxl345_config_t acc_params;
#endif

retry:
    accAlign = ALIGN_DEFAULT;

    switch (accHardwareToUse) {
#ifdef USE_FAKE_ACC
        default:
            if (fakeAccDetect(&acc)) {
                accHardware = ACC_FAKE;
                if (accHardwareToUse == ACC_FAKE)
                    break;
            }
#endif
        case ACC_NONE: // disable ACC
            sensorsClear(SENSOR_ACC);
            break;
        case ACC_DEFAULT: // autodetect
#ifdef USE_ACC_ADXL345
        case ACC_ADXL345: // ADXL345
            acc_params.useFifo = false;
            acc_params.dataRate = 800; // unused currently
#ifdef NAZE
            if (hardwareRevision < NAZE32_REV5 && adxl345Detect(&acc_params, &acc)) {
#else
            if (adxl345Detect(&acc_params, &acc)) {
#endif
#ifdef ACC_ADXL345_ALIGN
                accAlign = ACC_ADXL345_ALIGN;
#endif
                accHardware = ACC_ADXL345;
                if (accHardwareToUse == ACC_ADXL345)
                    break;
            }
            ; // fallthrough
#endif
#ifdef USE_ACC_LSM303DLHC
        case ACC_LSM303DLHC:
            if (lsm303dlhcAccDetect(&acc)) {
#ifdef ACC_LSM303DLHC_ALIGN
                accAlign = ACC_LSM303DLHC_ALIGN;
#endif
                accHardware = ACC_LSM303DLHC;
                if (accHardwareToUse == ACC_LSM303DLHC)
                    break;
            }
            ; // fallthrough
#endif
#ifdef USE_ACC_MPU6050
        case ACC_MPU6050: // MPU6050
            if (mpu6050AccDetect(selectMPU6050Config(), &acc)) {
#ifdef ACC_MPU6050_ALIGN
                accAlign = ACC_MPU6050_ALIGN;
#endif
                accHardware = ACC_MPU6050;
                if (accHardwareToUse == ACC_MPU6050)
                    break;
            }
            ; // fallthrough
#endif
#ifdef USE_ACC_MMA8452
        case ACC_MMA8452: // MMA8452
#ifdef NAZE
            // Not supported with this frequency
            if (hardwareRevision < NAZE32_REV5 && mma8452Detect(&acc)) {
#else
            if (mma8452Detect(&acc)) {
#endif
#ifdef ACC_MMA8452_ALIGN
                accAlign = ACC_MMA8452_ALIGN;
#endif
                accHardware = ACC_MMA8452;
                if (accHardwareToUse == ACC_MMA8452)
                    break;
            }
            ; // fallthrough
#endif
#ifdef USE_ACC_BMA280
        case ACC_BMA280: // BMA280
            if (bma280Detect(&acc)) {
#ifdef ACC_BMA280_ALIGN
                accAlign = ACC_BMA280_ALIGN;
#endif
                accHardware = ACC_BMA280;
                if (accHardwareToUse == ACC_BMA280)
                    break;
            }
            ; // fallthrough
#endif
#ifdef USE_ACC_SPI_MPU6000
        case ACC_SPI_MPU6000:
            if (mpu6000SpiAccDetect(&acc)) {
#ifdef ACC_SPI_MPU6000_ALIGN
                accAlign = ACC_SPI_MPU6000_ALIGN;
#endif
                accHardware = ACC_SPI_MPU6000;
                if (accHardwareToUse == ACC_SPI_MPU6000)
                    break;
            }
            ; // fallthrough
#endif
#ifdef USE_ACC_SPI_MPU6500
        case ACC_SPI_MPU6500:
#ifdef NAZE
            if (hardwareRevision == NAZE32_SP && mpu6500SpiAccDetect(&acc)) {
#else
            if (mpu6500SpiAccDetect(&acc)) {
#endif
#ifdef ACC_SPI_MPU6500_ALIGN
                accAlign = ACC_SPI_MPU6500_ALIGN;
#endif
                accHardware = ACC_SPI_MPU6500;
                if (accHardwareToUse == ACC_SPI_MPU6500)
                    break;
            }
            ; // fallthrough
#endif
            ; // prevent compiler error
    }

    // Found anything? Check if error or ACC is really missing.
    if (accHardware == ACC_DEFAULT) {
        if (accHardwareToUse > ACC_DEFAULT && accHardwareToUse < ACC_NONE) {
            // Nothing was found and we have a forced sensor that isn't present.
            accHardwareToUse = ACC_DEFAULT;
            goto retry;
        } else {
            // No ACC was detected
            sensorsClear(SENSOR_ACC);
        }
    }
}

static void detectBaro()
{
    // Detect what pressure sensors are available. baro->update() is set to sensor-specific update function

    #ifdef BARO
#ifdef USE_BARO_BMP085

    const bmp085Config_t *bmp085Config = NULL;

#if defined(BARO_XCLR_GPIO) && defined(BARO_EOC_GPIO)
    static const bmp085Config_t defaultBMP085Config = {
            .gpioAPB2Peripherals = BARO_APB2_PERIPHERALS,
            .xclrGpioPin = BARO_XCLR_PIN,
            .xclrGpioPort = BARO_XCLR_GPIO,
            .eocGpioPin = BARO_EOC_PIN,
            .eocGpioPort = BARO_EOC_GPIO
    };
    bmp085Config = &defaultBMP085Config;
#endif

#ifdef NAZE
    if (hardwareRevision == NAZE32) {
        bmp085Disable(bmp085Config);
    }
#endif

#endif

#ifdef USE_BARO_MS5611
    if (ms5611Detect(&baro)) {
        return;
    }
#endif

#ifdef USE_BARO_BMP085
    if (bmp085Detect(bmp085Config, &baro)) {
        return;
    }
#endif
#endif
    sensorsClear(SENSOR_BARO);
}

static void detectMag(uint8_t magHardwareToUse)
{
#ifdef USE_MAG_HMC5883
    static hmc5883Config_t *hmc5883Config = 0;

#ifdef NAZE
    hmc5883Config_t nazeHmc5883Config;

    if (hardwareRevision < NAZE32_REV5) {
        nazeHmc5883Config.gpioAPB2Peripherals = RCC_APB2Periph_GPIOB;
        nazeHmc5883Config.gpioPin = Pin_12;
        nazeHmc5883Config.gpioPort = GPIOB;
    } else {
        nazeHmc5883Config.gpioAPB2Peripherals = RCC_APB2Periph_GPIOC;
        nazeHmc5883Config.gpioPin = Pin_14;
        nazeHmc5883Config.gpioPort = GPIOC;
    }

    hmc5883Config = &nazeHmc5883Config;
#endif

#ifdef SPRACINGF3
    hmc5883Config_t spRacingF3Hmc5883Config = {
        .gpioAHBPeripherals = RCC_AHBPeriph_GPIOC,
        .gpioPin = Pin_14,
        .gpioPort = GPIOC
    };

    hmc5883Config = &spRacingF3Hmc5883Config;
#endif

#endif

retry:

    magAlign = ALIGN_DEFAULT;

    switch(magHardwareToUse) {
        case MAG_NONE: // disable MAG
            sensorsClear(SENSOR_MAG);
            break;
        case MAG_DEFAULT: // autodetect

#ifdef USE_MAG_HMC5883
        case MAG_HMC5883:
            if (hmc5883lDetect(&mag, hmc5883Config)) {
#ifdef MAG_HMC5883_ALIGN
                magAlign = MAG_HMC5883_ALIGN;
#endif
                magHardware = MAG_HMC5883;
                if (magHardwareToUse == MAG_HMC5883)
                    break;

            }
            ; // fallthrough
#endif

#ifdef USE_MAG_AK8975
        case MAG_AK8975:
            if (ak8975detect(&mag)) {

#ifdef MAG_AK8975_ALIGN
                magAlign = MAG_AK8975_ALIGN;
#endif
                magHardware = MAG_AK8975;
                if (magHardwareToUse == MAG_AK8975)
                    break;
            }
            ; // fallthrough
#endif
            ; // prevent compiler error.
    }

    if (magHardware == MAG_DEFAULT) {
        if (magHardwareToUse > MAG_DEFAULT && magHardwareToUse < MAG_NONE) {
            // Nothing was found and we have a forced sensor that isn't present.
            magHardwareToUse = MAG_DEFAULT;
            goto retry;
        } else {
            // No MAG was detected
            sensorsClear(SENSOR_MAG);
        }
    }

}

void reconfigureAlignment(sensorAlignmentConfig_t *sensorAlignmentConfig)
{
    if (sensorAlignmentConfig->gyro_align != ALIGN_DEFAULT) {
        gyroAlign = sensorAlignmentConfig->gyro_align;
    }
    if (sensorAlignmentConfig->acc_align != ALIGN_DEFAULT) {
        accAlign = sensorAlignmentConfig->acc_align;
    }
    if (sensorAlignmentConfig->mag_align != ALIGN_DEFAULT) {
        magAlign = sensorAlignmentConfig->mag_align;
    }
}

bool sensorsAutodetect(sensorAlignmentConfig_t *sensorAlignmentConfig, uint16_t gyroLpf, uint8_t accHardwareToUse, uint8_t magHardwareToUse, int16_t magDeclinationFromConfig)
{
    int16_t deg, min;
    memset(&acc, sizeof(acc), 0);
    memset(&gyro, sizeof(gyro), 0);

    if (!detectGyro(gyroLpf)) {
        return false;
    }
    sensorsSet(SENSOR_GYRO);
    detectAcc(accHardwareToUse);
    detectBaro();


    // Now time to init things, acc first
    if (sensors(SENSOR_ACC))
        acc.init();
    // this is safe because either mpu6050 or mpu3050 or lg3d20 sets it, and in case of fail, we never get here.
    gyro.init();

    detectMag(magHardwareToUse);

    reconfigureAlignment(sensorAlignmentConfig);

    // FIXME extract to a method to reduce dependencies, maybe move to sensors_compass.c
    if (sensors(SENSOR_MAG)) {
        // calculate magnetic declination
        deg = magDeclinationFromConfig / 100;
        min = magDeclinationFromConfig % 100;

        magneticDeclination = (deg + ((float)min * (1.0f / 60.0f))) * 10; // heading is in 0.1deg units
    } else {
        magneticDeclination = 0.0f; // TODO investigate if this is actually needed if there is no mag sensor or if the value stored in the config should be used.
    }

    return true;
}

