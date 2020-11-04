/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

/**@file nrf_fmfu.h
 *
 */
#ifndef NRF_FMFU_H__
#define NRF_FMFU_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(__GNUC__) || (__GNUC__ == 0)
typedef int32_t ssize_t;
#else
#include <sys/types.h>
#ifdef __SES_ARM
typedef int32_t ssize_t;
#endif
#endif

/**@addtogroup nrf_mfu_api_utils
 *@{
 */

/**@brief Function return codes. */
#define NRF_FMFU_RET_SUCCESS		 0  /**< SUCCESS. */
#define NRF_FMFU_RET_IPC_FAULT_EVENT	 -1 /**< The modem signaled a fault on the fault IPC channel (IPCEVENT_FAULT_RECEIVE_CHANNEL). */
#define NRF_FMFU_RET_UNEXPECTED_RESPONSE -2 /**< The modem response code in the RPC buffer (rpc_buffer->response.id) did not match the expected value. */
#define NRF_FMFU_RET_COMMAND_FAILED	 -3 /**< The modem replied with MODEM_RPC_RESP_CMD_ERROR to an RPC command. */
#define NRF_FMFU_RET_COMMAND_FAULT	 -4 /**< The modem replied with MODEM_RPC_RESP_UNKNOWN_CMD to an RPC command. */
#define NRF_FMFU_RET_TIMEOUT		 -5 /**< Timout while waiting for modem to respond on IPC channel. */
#define NRF_FMFU_RET_INVALID_ARGUMENT	 -6 /**< An invalid argument was passed to the function. */
#define NRF_FMFU_RET_INVALID_OPERATION	 -7 /**< The function/operation is not allowed with the current modem state. */

/**@brief Modem	states */
#define NRF_FMFU_MODEM_STATE_UNINITIALIZED	    1 /**< Modem is not initialized. */
#define NRF_FMFU_MODEM_STATE_WAITING_FOR_BOOTLOADER 2 /**< Modem is waiting the booloader. */
#define NRF_FMFU_MODEM_STATE_READY_FOR_IPC_COMMANDS 3 /**< Modem is ready for firmware upload. */
#define NRF_FMFU_MODEM_STATE_BAD		    4 /**< Modem is in error state. */

/**@brief
 * Structure for passing firmware data.
 * One piece of contiguous firmware is split into smaller nrf_fmfu_memory_chunk_t chunks.
 */
struct nrf_fmfu_memory_chunk_t {
	uint32_t target_address; /**< Destination address for the data (read by the modem). Not used for bootloader. */
	uint32_t data_len;	 /**< Chunk data length (bytes). */
	uint8_t *data;		 /**< Chunk data. */
};

/**@brief Buffer length for digest and uuid data. */
#define NRF_FMFU_DIGEST_BUFFER_LEN 32
#define NRF_FMFU_UUID_BUFFER_LEN   36

/**@brief Structure for storing 256-bit digest/hash replies. Endianess not converted. */
struct nrf_fmfu_digest_buffer_t {
	uint8_t data[NRF_FMFU_DIGEST_BUFFER_LEN];
};

/**@brief Structure for storing	modem UUID response. */
struct nrf_fmfu_uuid_t {
	uint8_t data[NRF_FMFU_UUID_BUFFER_LEN];
};

/**@brief
 * Function to set modem in DFU/RPC mode.
 *
 * @note Call once before MFU operation. If the modem goes to a bad state, this can be called again to re-initialize.
 *       The root key digest response of the modem is put in the digest_buffer structure.
 *       The size reserved for modem RPC buffer is put in the modem_buffer_length argument.
 *       If success, the modem will be in NRF_FMFU_RPC_MODEM_STATE_WAITING_FOR_BOOTLOADER state.
 *
 * @param[out]  digest_buffer               Pointer to the buffer to store digest hash.
 * @param[out]  modem_buffer_length         Pointer to the buffer to store reserved RPC buffer length.
 *
 * @retval  NRF_FMFU_RET_SUCCESS On success.
 * @retval  NRF_FMFU_RET_INVALID_ARGUMENT When input parameter is NULL.
 * @retval  NRF_FMFU_RET_IPC_FAULT_EVENT When modem signaled fault.
 * @retval  NRF_FMFU_RET_UNEXPECTED_RESPONSE When modem replies with unexpected response.
 */
int nrf_fmfu_init(struct nrf_fmfu_digest_buffer_t *digest_buffer,
		  uint32_t *modem_buffer_length);

/**@brief
 * Function to finalize firmware update process.
 * Call once after DFU operation to set the modem back to normal mode.
 *
 * @retval  NRF_FMFU_RET_SUCCESS On success.
 * @retval  NRF_FMFU_RET_UNEXPECTED_RESPONSE When modem cannot be restarted.
 */
int nrf_fmfu_end(void);

/**@brief
 * Function for writing a memory chunk to the modem.
 *
 * @note Call after nrf_mfu_init to upload modem firmware segments.
 *       Bootloader segment must be uploaded first which sets the modem to
 *       NRF_FMFU_MODEM_STATE_READY_FOR_IPC_COMMANDS state. Firmware segments can be
 *       uploaded after successful bootloader upload.
 *       Use nrf_fmfu_transfer_start and nrf_fmfu_transfer_end at the start
 *       and end of each segment upload.
 *       Call nrf_fmfu_end after uploading all segments to finalize the update process.
 *
 * @param[in]   memory_chunk          Pointer to the buffer where chunk data is stored.
 *
 * @retval  NRF_FMFU_RET_SUCCESS On success.
 * @retval  NRF_FMFU_RET_INVALID_ARGUMENT When input parameter is NULL.
 * @retval  NRF_FMFU_RET_INVALID_OPERATION When modem is in wrong state.
 * @retval  NRF_FMFU_RET_COMMAND_FAULT When modem replies with unknown response.
 * @retval  NRF_FMFU_RET_COMMAND_FAILED When modem replies with error response.
 * @retval  NRF_FMFU_RET_UNEXPECTED_RESPONSE When modem replies with unexpected response.
 * @retval  NRF_FMFU_RET_IPC_FAULT_EVENT When modem signaled fault.
 * @retval  NRF_FMFU_RET_TIMEOUT When got timeout on waiting the response from modem.
 */
int nrf_fmfu_write_memory_chunk(struct nrf_fmfu_memory_chunk_t *memory_chunk);

/**@brief
 * Function for starting a data chunk download.
 *
 * @retval  NRF_FMFU_RET_SUCCESS On success.
 * @retval  NRF_FMFU_RET_INVALID_OPERATION When modem is in wrong state.
 */
int nrf_fmfu_transfer_start(void);

/**@brief
 * Function for ending a data chunk download.
 *
 * @retval  NRF_FMFU_RET_SUCCESS On success.
 * @retval  NRF_FMFU_RET_INVALID_OPERATION When modem is in wrong state.
 * @retval  NRF_FMFU_RET_COMMAND_FAULT When modem replies with unknown response.
 * @retval  NRF_FMFU_RET_COMMAND_FAILED When modem replies with error response.
 * @retval  NRF_FMFU_RET_UNEXPECTED_RESPONSE When modem replies with unexpected response.
 */
int nrf_fmfu_transfer_end(void);

/**@brief
 * Function for reading a digest hash data from the modem.
 *
 * @param[in]   start_address   Start address.
 * @param[in]   end_address     End address.
 * @param[out]  digest_buffer   Pointer to the buffer to store digest hash data.
 *
 * @retval  NRF_FMFU_RET_SUCCESS On success.
 * @retval  NRF_FMFU_RET_INVALID_ARGUMENT When input parameter is NULL.
 * @retval  NRF_FMFU_RET_INVALID_OPERATION When modem is in wrong state.
 * @retval  NRF_FMFU_RET_COMMAND_FAULT When modem replies with unknown response.
 * @retval  NRF_FMFU_RET_COMMAND_FAILED When modem replies with error response.
 * @retval  NRF_FMFU_RET_UNEXPECTED_RESPONSE When modem replies with unexpected response.
 * @retval  NRF_FMFU_RET_IPC_FAULT_EVENT When modem signaled fault.
 * @retval  NRF_FMFU_RET_TIMEOUT When got timeout on waiting the response from modem.
 */
int nrf_fmfu_get_memory_hash(uint32_t start_address,
			     uint32_t end_address,
			     struct nrf_fmfu_digest_buffer_t *digest_buffer);

/**@brief
 * Function for reading an uuid data from the modem.
 *
 * @param[out]  modem_uuid      Pointer to the buffer to store uuid data.
 *
 * @retval  NRF_FMFU_RET_SUCCESS On success.
 * @retval  NRF_FMFU_RET_INVALID_ARGUMENT When input parameter is NULL.
 * @retval  NRF_FMFU_RET_INVALID_OPERATION When modem is in wrong state.
 * @retval  NRF_FMFU_RET_COMMAND_FAULT When modem replies with unknown response.
 * @retval  NRF_FMFU_RET_COMMAND_FAILED When modem replies with error response.
 * @retval  NRF_FMFU_RET_UNEXPECTED_RESPONSE When modem replies with unexpected response.
 * @retval  NRF_FMFU_RET_IPC_FAULT_EVENT When modem signaled fault.
 * @retval  NRF_FMFU_RET_TIMEOUT When got timeout on waiting the response from modem.
 */
int nrf_fmfu_get_uuid(struct nrf_fmfu_uuid_t *modem_uuid);

/**@brief
 * Function for reading modem state.
 *
 * @retval  NRF_FMFU_MODEM_STATE_UNINITIALIZED When modem is not initialized.
 * @retval  NRF_FMFU_MODEM_STATE_WAITING_FOR_BOOTLOADER When modem is ready to receive bootloader.
 * @retval  NRF_FMFU_MODEM_STATE_READY_FOR_IPC_COMMANDS When modem is ready to receive firmware.
 * @retval  NRF_FMFU_MODEM_STATE_BAD When modem is in error state.
 */
int nrf_fmfu_get_modem_state(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_FMFU_H__

/**@} */
