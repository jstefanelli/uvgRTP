#pragma once

#include "dispatch.hh"
#include "queue.hh"
#include "rtp.hh"
#include "socket.hh"

namespace kvz_rtp {

    class frame_queue;
    class dispatcher;

    class sender {
        public:
            sender(kvz_rtp::socket& socket, rtp_ctx_conf& conf, rtp_format_t fmt, kvz_rtp::rtp *rtp);
            ~sender();

            /* Initialize the RTP sender by adjusting UDP buffer size,
             * creating a frame queue and possibly creating a dispatcher 
             *
             * Return RTP_OK on success
             * Return RTP_MEMORY_ERROR if allocation failed */
            rtp_error_t init();

            /* TODO:  */
            rtp_error_t destroy();

            /* Split "data" into 1500 byte chunks and send them to remote
             *
             * NOTE: If SCD has been enabled, calling this version of push_frame()
             * requires either that the caller has given a deallocation callback to
             * SCD OR that "flags" contains flags "RTP_COPY"
             *
             * Return RTP_OK success
             * Return RTP_INVALID_VALUE if one of the parameters are invalid
             * Return RTP_MEMORY_ERROR  if the data chunk is too large to be processed
             * Return RTP_SEND_ERROR    if kvzRTP failed to send the data to remote
             * Return RTP_GENERIC_ERROR for any other error condition */
            rtp_error_t push_frame(uint8_t *data, size_t data_len, int flags);

            /* Same as push_frame() defined above but no callback nor RTP_COPY must be provided
             * One must call this like: push_frame(std::move(data), ...) to give ownership of the
             * memory to kvzRTP */
            rtp_error_t push_frame(std::unique_ptr<uint8_t[]> data, size_t data_len, int flags);

            /* Get pointer to the frame queue */
            kvz_rtp::frame_queue *get_frame_queue();

            /* Install deallocation hook to frame queue */
            void install_dealloc_hook(void (*dealloc_hook)(void *));

            /* Get reference to the underlying socket object */
            kvz_rtp::socket & get_socket();

            /* Get pointer to RTP context where all clocking informatoin,
             * SSRC, sequence number etc. are stored */
            kvz_rtp::rtp *get_rtp_ctx();

        private:
            kvz_rtp::socket socket_;
            kvz_rtp::rtp *rtp_;
            rtp_ctx_conf conf_;
            rtp_format_t fmt_;

            sockaddr_in addr_out_;

            kvz_rtp::frame_queue *fqueue_;
            kvz_rtp::dispatcher  *dispatcher_;
    };
};
