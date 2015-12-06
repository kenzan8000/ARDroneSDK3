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
 * @file ARSTREAM_Reader2.h
 * @brief Stream reader on network (v2)
 * @date 04/16/2015
 * @author aurelien.barre@parrot.com
 */

#ifndef _ARSTREAM_READER2_H_
#define _ARSTREAM_READER2_H_

/*
 * System Headers
 */
#include <inttypes.h>

/*
 * ARSDK Headers
 */
#include <libARStream/ARSTREAM_Error.h>

/*
 * Types
 */

/**
 * @brief Causes for FrameComplete callback
 */
typedef enum {
    ARSTREAM_READER2_CAUSE_NALU_COMPLETE = 0, /**< Frame is complete (no error) */
    ARSTREAM_READER2_CAUSE_NALU_BUFFER_TOO_SMALL, /**< Frame buffer is too small for the frame on the network */
    ARSTREAM_READER2_CAUSE_NALU_COPY_COMPLETE, /**< Copy of previous frame buffer is complete (called only after ARSTREAM_READER2_CAUSE_FRAME_TOO_SMALL) */
    ARSTREAM_READER2_CAUSE_CANCEL, /**< Reader is closing, so buffer is no longer used */
    ARSTREAM_READER2_CAUSE_MAX,
} eARSTREAM_READER2_CAUSE;

/**
 * @brief H.264 slice types
 */
typedef enum {
    ARSTREAM_READER2_H264_NON_VCL = 0, /**< Non-VCL NAL unit */
    ARSTREAM_READER2_H264_SLICE_I, /**< I-slice */
    ARSTREAM_READER2_H264_SLICE_P, /**< P-slice */
    ARSTREAM_READER2_H264_SLICE_TYPE_MAX,
} eARSTREAM_READER2_H264_SLICE_TYPE;

/**
 * @brief Callback called when a new frame is ready in a buffer
 *
 * @param[in] cause Describes why this callback was called
 * @param[in] framePointer Pointer to the frame buffer which was received
 * @param[in] frameSize Used size in framePointer buffer
 * @param[in] isFlushFrame Boolean-like (0-1) flag telling if the complete frame was a flush frame (typically an I-Frame) for the sender
 * @param[in] numberOfSkippedFrames Number of frames which were skipped between the previous call and this one. (Usually 0)
 * @param[in] numberOfMissingFragments Number of fragments missing from the frame. (Usually 0)
 * @param[in] totalFragments Total number of expected fragments for the frame.
 * @param[inout] newBufferCapacity Capacity of the next buffer to use
 * @param[in] custom Custom pointer passed during ARSTREAM_Reader2_New
 *
 * @return address of a new buffer which will hold the next frame
 *
 * @note If cause is ARSTREAM_READER2_CAUSE_FRAME_COMPLETE, framePointer contains a valid frame.
 * @note If cause is ARSTREAM_READER2_CAUSE_FRAME_INCOMPLETE, framePointer contains an incomplete frame that may be partly decodable.
 * @note If cause is ARSTREAM_READER2_CAUSE_FRAME_TOO_SMALL, datas will be copied into the new frame. Old frame buffer will still be in use until the callback is called again with ARSTREAM_READER2_CAUSE_COPY_COMPLETE cause. If the new frame is still too small, the callback will be called again, until a suitable buffer is provided. newBufferCapacity holds a suitable capacity for the new buffer, but still has to be updated by the application.
 * @note If cause is ARSTREAM_READER2_CAUSE_COPY_COMPLETE, the return value and newBufferCapacity are unused. If numberOfSkippedFrames is non-zero, then the current frame will be skipped (usually because the buffer returned after the ARSTREAM_READER2_CAUSE_FRAME_TOO_SMALL was smaller than the previous buffer).
 * @note If cause is ARSTREAM_READER2_CAUSE_CANCEL, the return value and newBufferCapacity are unused
 *
 * @warning If the cause is ARSTREAM_READER2_CAUSE_FRAME_TOO_SMALL, returning a buffer shorter than 'frameSize' will cause the library to skip the current frame
 * @warning In any case, returning a NULL buffer is not supported.
 */
typedef uint8_t* (*ARSTREAM_Reader2_NaluCallback_t) (eARSTREAM_READER2_CAUSE cause, uint8_t *naluBuffer, int naluSize, uint64_t auTimestamp, int isFirstNaluInAu, int isLastNaluInAu, int missingPacketsBefore, eARSTREAM_READER2_H264_SLICE_TYPE sliceType, int *newNaluBufferSize, void *custom);

typedef struct ARSTREAM_Reader2_Config_t {
    const char *ifaceAddr;
    const char *recvAddr;
    int recvPort;
    ARSTREAM_Reader2_NaluCallback_t naluCallback;
    int maxPacketSize;
    int insertStartCodes;
} ARSTREAM_Reader2_Config_t;

typedef struct ARSTREAM_Reader2_Resender_Config_t {
    const char *ifaceAddr;
    const char *sendAddr;
    int sendPort;
    int maxPacketSize;
    int targetPacketSize;
    int maxBitrate;
    int maxLatencyMs;
    int maxNetworkLatencyMs;
} ARSTREAM_Reader2_Resender_Config_t;

/**
 * @brief An ARSTREAM_Reader2_t instance allow reading streamed frames from a network
 */
typedef struct ARSTREAM_Reader2_t ARSTREAM_Reader2_t;

typedef struct ARSTREAM_Reader2_Resender_t ARSTREAM_Reader2_Resender_t;

/*
 * Functions declarations
 */

/**
 * @brief Creates a new ARSTREAM_Reader2_t
 * @warning This function allocates memory. An ARSTREAM_Reader2_t muse be deleted by a call to ARSTREAM_Reader2_Delete
 *
 * @param[in] manager Pointer to a valid and connected ARNETWORK_Manager_t, which will be used to stream frames
 * @param[in] dataBufferID ID of a StreamDataBuffer available within the manager
 * @param[in] ackBufferID ID of a StreamAckBuffer available within the manager
 * @param[in] callback The callback which will be called every time a new frame is available
 * @param[in] frameBuffer The adress of the first frameBuffer to use
 * @param[in] frameBufferSize The length of the frameBuffer (to avoid overflow)
 * @param[in] maxFragmentSize Maximum allowed size for a video data fragment. Video frames larger that will be fragmented.
 * @param[in] custom Custom pointer which will be passed to callback
 * @param[out] error Optionnal pointer to an eARSTREAM_ERROR to hold any error information
 * @return A pointer to the new ARSTREAM_Reader2_t, or NULL if an error occured
 * @see ARSTREAM_Reader2_InitStreamDataBuffer()
 * @see ARSTREAM_Reader2_InitStreamAckBuffer()
 * @see ARSTREAM_Reader2_StopReader()
 * @see ARSTREAM_Reader2_Delete()
 */
ARSTREAM_Reader2_t* ARSTREAM_Reader2_New (ARSTREAM_Reader2_Config_t *config, uint8_t *naluBuffer, int naluBufferSize, void *custom, eARSTREAM_ERROR *error);

/**
 * @brief Stops a running ARSTREAM_Reader2_t
 * @warning Once stopped, an ARSTREAM_Reader2_t can not be restarted
 *
 * @param[in] reader The ARSTREAM_Reader2_t to stop
 *
 * @note Calling this function multiple times has no effect
 */
void ARSTREAM_Reader2_StopReader (ARSTREAM_Reader2_t *reader);

/**
 * @brief Deletes an ARSTREAM_Reader2_t
 * @warning This function should NOT be called on a running ARSTREAM_Reader2_t
 *
 * @param reader Pointer to the ARSTREAM_Reader2_t * to delete
 *
 * @return ARSTREAM_OK if the ARSTREAM_Reader2_t was deleted
 * @return ARSTREAM_ERROR_BUSY if the ARSTREAM_Reader2_t is still busy and can not be stopped now (probably because ARSTREAM_Reader2_StopReader() was not called yet)
 * @return ARSTREAM_ERROR_BAD_PARAMETERS if reader does not point to a valid ARSTREAM_Reader2_t
 *
 * @note The library use a double pointer, so it can set *reader to NULL after freeing it
 */
eARSTREAM_ERROR ARSTREAM_Reader2_Delete (ARSTREAM_Reader2_t **reader);

/**
 * @brief Runs the data loop of the ARSTREAM_Reader2_t
 * @warning This function never returns until ARSTREAM_Reader2_StopReader() is called. Thus, it should be called on its own thread
 * @post Stop the ARSTREAM_Reader2_t by calling ARSTREAM_Reader2_StopReader() before joining the thread calling this function
 * @param[in] ARSTREAM_Reader2_t_Param A valid (ARSTREAM_Reader2_t *) casted as a (void *)
 */
void* ARSTREAM_Reader2_RunRecvThread (void *ARSTREAM_Reader2_t_Param);

/**
 * @brief Runs the acknowledge loop of the ARSTREAM_Reader2_t
 * @warning This function never returns until ARSTREAM_Reader2_StopReader() is called. Thus, it should be called on its own thread
 * @post Stop the ARSTREAM_Reader_t by calling ARSTREAM_Reader2_StopReader() before joining the thread calling this function
 * @param[in] ARSTREAM_Reader2_t_Param A valid (ARSTREAM_Reader2_t *) casted as a (void *)
 */
void* ARSTREAM_Reader2_RunSendThread (void *ARSTREAM_Reader2_t_Param);

/**
 * @brief Gets the custom pointer associated with the reader
 * @param[in] reader The ARSTREAM_Reader2_t
 * @return The custom pointer associated with this reader, or NULL if reader does not point to a valid reader
 */
void* ARSTREAM_Reader2_GetCustom (ARSTREAM_Reader2_t *reader);

eARSTREAM_ERROR ARSTREAM_Reader2_GetMonitoring(ARSTREAM_Reader2_t *reader, uint64_t startTime, uint32_t timeIntervalUs, uint32_t *realTimeIntervalUs, uint32_t *receptionTimeJitter,
                                               uint32_t *bytesReceived, uint32_t *meanPacketSize, uint32_t *packetSizeStdDev, uint32_t *packetsReceived, uint32_t *packetsMissed);

ARSTREAM_Reader2_Resender_t* ARSTREAM_Reader2_Resender_New (ARSTREAM_Reader2_t *reader, ARSTREAM_Reader2_Resender_Config_t *config, eARSTREAM_ERROR *error);

void ARSTREAM_Reader2_Resender_Stop (ARSTREAM_Reader2_Resender_t *resender);

eARSTREAM_ERROR ARSTREAM_Reader2_Resender_Delete (ARSTREAM_Reader2_Resender_t **resender);

void* ARSTREAM_Reader2_Resender_RunSendThread (void *ARSTREAM_Reader2_Resender_t_Param);

void* ARSTREAM_Reader2_Resender_RunRecvThread (void *ARSTREAM_Reader2_Resender_t_Param);

#endif /* _ARSTREAM_READER2_H_ */
