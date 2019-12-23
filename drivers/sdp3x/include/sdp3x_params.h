/*
 * SDP3x_params.h
 *
 *  Created on: Oct 23, 2019
 *      Author: ehmen
 */

#ifndef SDP3X_PARAMS_H_
#define SDP3X_PARAMS_H_

#include "board.h"
#include "sdp3x.h"
#include "saul_reg.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name    SDP3x I2C addresses
 * @{
 */
#define SDP3X_ADDR1                   (0x21) /* 7 bit address */
#define SDP3X_ADDR2                   (0x22) /* 7 bit address */
#define SDP3X_ADDR3                   (0x23) /* 7 bit address */
/** @} */

/**
 * @name    SDP3x Models
 * @{
 */
#define SDP3X_MODEL_31                1
#define SDP3X_MODEL_32                2

#define SDP31_PRODUCT_NO_BYTE_0       0x03
#define SDP31_PRODUCT_NO_BYTE_1       0x01
#define SDP31_PRODUCT_NO_BYTE_2       0x01
#define SDP31_PRODUCT_NO_BYTE_3       0x01
/** @} */

/**
 * @name    Set default configuration parameters for the SDP3X
 * @{
 */
#ifndef SDP3X_PARAM_I2C_DEV
#define SDP3X_PARAM_I2C_DEV         I2C_DEV(0)
#endif
#ifndef SDP3X_PARAM_I2C_ADDR
#define SDP3X_PARAM_I2C_ADDR        SDP3X_ADDR1
#endif
#ifndef SDP3X_PARAM_MODEL
#define SDP3X_PARAM_MODEL    SDP3X_MODEL_31
#endif

#ifndef SDP3X_PARAMS
#define SDP3X_PARAMS                { .i2c_dev      = SDP3X_PARAM_I2C_DEV,  \
                                       .i2c_addr     = SDP3X_PARAM_I2C_ADDR, \
                                       .model = SDP3X_PARAM_MODEL }
#endif
#ifndef SDP3X_SAUL_INFO
#define SDP3X_SAUL_INFO             { .name = "sdp3x" }
#endif
/**@}*/

/**
 * @brief   Configure SDP3X
 */

//TODO fill in board params here with more than one sensor
static const sdp3x_params_t sdp3x_params[] =
{
    SDP3X_PARAMS
};

/**
 * @brief   Get the number of configured SDP3X devices
 */
#define SDP3X_NUMOF       ARRAY_SIZE(sdp3x_params)

/**
 * @brief   Configure SAUL registry entries
 */
static const saul_reg_info_t sdp3x_saul_info[SDP3X_NUMOF] =
{
    SDP3X_SAUL_INFO
};

#ifdef __cplusplus
}
#endif


#endif /* SDP3X_PARAMS_H_ */
