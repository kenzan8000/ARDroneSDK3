/*
    Copyright (C) 2014 Parrot SA

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the 
      distribution.
    * Neither the name of Parrot nor the names
      of its contributors may be used to endorse or promote products
      derived from this software without specific prior written
      permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
    "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
    LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
    FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
    COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
    INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
    BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
    OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
    AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
    OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
    OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
    SUCH DAMAGE.
*/
/**
 * @file ARSTREAM_Sender2.h
 * @brief Stream sender over network (v2)
 * @date 04/17/2015
 * @author aurelien.barre@parrot.com
 */

#ifndef _ARSTREAM_SENDER2_H_
#define _ARSTREAM_SENDER2_H_

/*
 * System Headers
 */
#include <inttypes.h>

/*
 * ARSDK Headers
 */
#include <libARStream/ARSTREAM_Error.h>

/*
 * Macros
 */

/*
 * Types
 */

/**
 * @brief Callback status values
 */
typedef enum {
    ARSTREAM_SENDER2_STATUS_SENT = 0, /**< Access Unit was sent */
    ARSTREAM_SENDER2_STATUS_CANCELLED, /**< Access Unit was cancelled (not sent or partly sent) */
    ARSTREAM_SENDER2_STATUS_MAX,
} eARSTREAM_SENDER2_STATUS;

/**
 * @brief Callback type for sender informations
 * This callback is called when a frame pointer is no longer needed by the library.
 * This can occur when a frame is acknowledged, cancelled, or if a network error happened.
 *
 * @param[in] status Why the call was made
 * @param[in] framePointer Pointer to the frame which was sent/cancelled
 * @param[in] frameSize Size, in bytes, of the frame
 * @param[in] custom Custom pointer passed during ARSTREAM_Sender2_New
 * @see eARSTREAM_SENDER2_STATUS
 */
typedef void (*ARSTREAM_Sender2_AuCallback_t)(eARSTREAM_SENDER2_STATUS status, void *auUserPtr, void *custom);

typedef void (*ARSTREAM_Sender2_NaluCallback_t)(eARSTREAM_SENDER2_STATUS status, void *naluUserPtr, void *custom);

typedef struct ARSTREAM_Sender2_Config_t {
    const char *ifaceAddr;
    const char *sendAddr;
    int sendPort;
    ARSTREAM_Sender2_AuCallback_t auCallback;
    ARSTREAM_Sender2_NaluCallback_t naluCallback;
    int naluFifoSize;
    int maxPacketSize;
    int targetPacketSize;
    int maxBitrate;
    int maxLatencyMs;
    int maxNetworkLatencyMs;
} ARSTREAM_Sender2_Config_t;

/**
 * @brief An ARSTREAM_Sender2_t instance allow streaming frames over a network
 */
typedef struct ARSTREAM_Sender2_t ARSTREAM_Sender2_t;



/*
 * Functions declarations
 */

/**
 * @brief Creates a new ARSTREAM_Sender2_t
 * @warning This function allocates memory. An ARSTREAM_Sender2_t muse be deleted by a call to ARSTREAM_Sender2_Delete
 *
 * @param[in] manager Pointer to a valid and connected ARNETWORK_Manager_t, which will be used to stream frames
 * @param[in] dataBufferID ID of a StreamDataBuffer available within the manager
 * @param[in] ackBufferID ID of a StreamAckBuffer available within the manager
 * @param[in] callback The status update callback which will be called every time the status of a send-frame is updated
 * @param[in] framesBufferSize Number of frames that the ARSTREAM_Sender2_t instance will be able to hold in queue
 * @param[in] maxFragmentSize Maximum allowed size for a video data fragment. Video frames larger that will be fragmented.
 * @param[in] maxNumberOfFragment number maximum of fragment of one frame.
 * @param[in] custom Custom pointer which will be passed to callback
 * @param[out] error Optionnal pointer to an eARSTREAM_ERROR to hold any error information
 * @return A pointer to the new ARSTREAM_Sender2_t, or NULL if an error occured
 *
 * @note framesBufferSize should be greater than the number of frames between two I-Frames
 *
 * @see ARSTREAM_Sender2_InitStreamDataBuffer()
 * @see ARSTREAM_Sender2_InitStreamAckBuffer()
 * @see ARSTREAM_Sender2_StopSender()
 * @see ARSTREAM_Sender2_Delete()
 */
ARSTREAM_Sender2_t* ARSTREAM_Sender2_New (ARSTREAM_Sender2_Config_t *config, void *custom, eARSTREAM_ERROR *error);

/**
 * @brief Stops a running ARSTREAM_Sender2_t
 * @warning Once stopped, an ARSTREAM_Sender2_t can not be restarted
 *
 * @param[in] sender The ARSTREAM_Sender2_t to stop
 *
 * @note Calling this function multiple times has no effect
 */
void ARSTREAM_Sender2_StopSender (ARSTREAM_Sender2_t *sender);

/**
 * @brief Deletes an ARSTREAM_Sender2_t
 * @warning This function should NOT be called on a running ARSTREAM_Sender2_t
 *
 * @param sender Pointer to the ARSTREAM_Sender2_t * to delete
 *
 * @return ARSTREAM_OK if the ARSTREAM_Sender2_t was deleted
 * @return ARSTREAM_ERROR_BUSY if the ARSTREAM_Sender2_t is still busy and can not be stopped now (probably because ARSTREAM_Sender2_StopSender() was not called yet)
 * @return ARSTREAM_ERROR_BAD_PARAMETERS if sender does not point to a valid ARSTREAM_Sender2_t
 *
 * @note The library use a double pointer, so it can set *sender to NULL after freeing it
 */
eARSTREAM_ERROR ARSTREAM_Sender2_Delete (ARSTREAM_Sender2_t **sender);

/**
 * @brief Sends a new frame
 *
 * @param[in] sender The ARSTREAM_Sender2_t which will try to send the frame
 * @param[in] frameBuffer pointer to the frame in memory
 * @param[in] frameSize size of the frame in memory
 * @param[in] flushPreviousFrames Boolean-like flag (0/1). If active, tells the sender to flush the frame queue when adding this frame.
 * @param[out] nbPreviousFrames Optionnal int pointer which will store the number of frames previously in the buffer (even if the buffer is flushed)
 * @return ARSTREAM_OK if no error happened
 * @return ARSTREAM_ERROR_BAD_PARAMETERS if the sender or frameBuffer pointer is invalid, or if frameSize is zero
 * @return ARSTREAM_ERROR_FRAME_TOO_LARGE if the frameSize is greater that the maximum frame size of the libARStream (typically 128000 bytes)
 * @return ARSTREAM_ERROR_QUEUE_FULL if the frame can not be added to queue. This value can not happen if flushPreviousFrames is active
 */
eARSTREAM_ERROR ARSTREAM_Sender2_SendNewNalu (ARSTREAM_Sender2_t *sender, uint8_t *naluBuffer, uint32_t naluSize, uint64_t auTimestamp, int isLastNaluInAu, void *auUserPtr, void *naluUserPtr);

/**
 * @brief Flushes all currently queued frames
 *
 * @param[in] sender The ARSTREAM_Sender2_t to be flushed.
 * @return ARSTREAM_OK if no error occured.
 * @return ARSTREAM_ERROR_BAD_PARAMETERS if the sender is invalid.
 */
eARSTREAM_ERROR ARSTREAM_Sender2_FlushNaluQueue (ARSTREAM_Sender2_t *sender);

/**
 * @brief Runs the data loop of the ARSTREAM_Sender2_t
 * @warning This function never returns until ARSTREAM_Sender2_StopSender() is called. Thus, it should be called on its own thread
 * @post Stop the ARSTREAM_Sender2_t by calling ARSTREAM_Sender2_StopSender() before joining the thread calling this function
 * @param[in] ARSTREAM_Sender2_t_Param A valid (ARSTREAM_Sender2_t *) casted as a (void *)
 */
void* ARSTREAM_Sender2_RunSendThread (void *ARSTREAM_Sender2_t_Param);

/**
 * @brief Runs the acknowledge loop of the ARSTREAM_Sender2_t
 * @warning This function never returns until ARSTREAM_Sender2_StopSender() is called. Thus, it should be called on its own thread
 * @post Stop the ARSTREAM_Sender2_t by calling ARSTREAM_Sender2_StopSender() before joining the thread calling this function
 * @param[in] ARSTREAM_Sender2_t_Param A valid (ARSTREAM_Sender2_t *) casted as a (void *)
 */
void* ARSTREAM_Sender2_RunRecvThread (void *ARSTREAM_Sender2_t_Param);

/**
 * @brief Gets the custom pointer associated with the sender
 * @param[in] sender The ARSTREAM_Sender2_t
 * @return The custom pointer associated with this sender, or NULL if sender does not point to a valid sender
 */
void* ARSTREAM_Sender2_GetCustom (ARSTREAM_Sender2_t *sender);

int ARSTREAM_Sender2_GetTargetPacketSize(ARSTREAM_Sender2_t *sender);

eARSTREAM_ERROR ARSTREAM_Sender2_SetTargetPacketSize(ARSTREAM_Sender2_t *sender, int targetPacketSize);

int ARSTREAM_Sender2_GetMaxBitrate(ARSTREAM_Sender2_t *sender);

int ARSTREAM_Sender2_GetMaxLatencyMs(ARSTREAM_Sender2_t *sender);

int ARSTREAM_Sender2_GetMaxNetworkLatencyMs(ARSTREAM_Sender2_t *sender);

eARSTREAM_ERROR ARSTREAM_Sender2_SetMaxBitrateAndLatencyMs(ARSTREAM_Sender2_t *sender, int maxBitrate, int maxLatencyMs, int maxNetworkLatencyMs);

eARSTREAM_ERROR ARSTREAM_Sender2_GetMonitoring(ARSTREAM_Sender2_t *sender, uint64_t startTime, uint32_t timeIntervalUs, uint32_t *realTimeIntervalUs, uint32_t *meanAcqToNetworkTime,
                                               uint32_t *acqToNetworkJitter, uint32_t *meanNetworkTime, uint32_t *networkJitter, uint32_t *bytesSent, uint32_t *meanPacketSize,
                                               uint32_t *packetSizeStdDev, uint32_t *packetsSent, uint32_t *bytesDropped, uint32_t *naluDropped);

#endif /* _ARSTREAM_SENDER2_H_ */
