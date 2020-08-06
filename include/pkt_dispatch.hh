#pragma once

#include <mutex>
#include <unordered_map>

#include "frame.hh"
#include "runner.hh"
#include "socket.hh"
#include "util.hh"

namespace uvg_rtp {

    typedef rtp_error_t (*packet_handler)(ssize_t, void *, int, uvg_rtp::frame::rtp_frame **);
    typedef rtp_error_t (*packet_handler_aux)(int, uvg_rtp::frame::rtp_frame **);

    struct packet_handlers {
        packet_handler primary;
        std::vector<packet_handler_aux> auxiliary;
    };

    class pkt_dispatcher : public runner {
        public:
            pkt_dispatcher();
            ~pkt_dispatcher();

            /* Install a primary handler for an incoming UDP datagram
             *
             * This handler is responsible for creating an operable RTP packet
             * that auxiliary handlers can work with.
             *
             * It is also responsible for validating the packet on a high level
             * (ZRTP checksum/RTP version etc) before passing it onto other handlers.
             *
             * Return a key on success that differentiates primary packet handlers
             * Return 0 "handler" is nullptr */
            uint32_t install_handler(packet_handler handler);

            /* Install auxiliary handler for the packet
             *
             * This handler is responsible for doing auxiliary operations on the packet
             * such as gathering sessions statistics data or decrypting the packet
             * It is called only after the primary handler of the auxiliary handler is called
             *
             * "key" is used to specify for which primary handlers for "handler"
             * An auxiliary handler can be installed to multiple primary handlers
             *
             * Return RTP_OK on success
             * Return RTP_INVALID_VALUE if "handler" is nullptr or if "key" is not valid */
            rtp_error_t install_aux_handler(uint32_t key, packet_handler_aux handler);

            /* Install receive hook for the RTP packet dispatcher
             *
             * Return RTP_OK on success
             * Return RTP_INVALID_VALUE if "hook" is nullptr */
            rtp_error_t install_receive_hook(void *arg, void (*hook)(void *, uvg_rtp::frame::rtp_frame *));

            /* Start the RTP packet dispatcher
             *
             * Return RTP_OK on success
             * Return RTP_MEMORY_ERROR if allocation of a thread object fails */
            rtp_error_t start(uvg_rtp::socket *socket, int flags);

            /* Stop the RTP packet dispatcher and wait until the receive loop is exited
             * to make sure that destroying the object in media_stream.cc is safe
             *
             * Return RTP_OK on success */
            rtp_error_t stop();

            /* Fetch frame from the frame queue that contains all received frame.
             * pull_frame() will block until there is a frame that can be returned.
             * If "timeout" is given, pull_frame() will block only for however long
             * that value tells it to.
             * If no frame is received within that time period, pull_frame() returns nullptr
             *
             * Return pointer to RTP frame on success
             * Return nullptr if operation timed out or an error occurred */
            uvg_rtp::frame::rtp_frame *pull_frame();
            uvg_rtp::frame::rtp_frame *pull_frame(size_t ms);

            /* Return reference to the map that holds all installed handlers */
            std::unordered_map<uint32_t, uvg_rtp::packet_handlers>& get_handlers();

            /* Return a processed RTP frame to user either through frame queue or receive hook */
            void return_frame(uvg_rtp::frame::rtp_frame *frame);

            /* Call auxiliary handlers of a primary handler */
            void call_aux_handlers(uint32_t key, int flags, uvg_rtp::frame::rtp_frame **frame);

            /* RTP packet dispatcher thread */
            static void runner(
                uvg_rtp::pkt_dispatcher *dispatcher,
                uvg_rtp::socket *socket,
                int flags,
                std::mutex *exit_mtx
            );

        private:
            std::unordered_map<uint32_t, packet_handlers> packet_handlers_;

            /* If receive hook has not been installed, frames are pushed to "frames_"
             * and they can be retrieved using pull_frame() */
            std::vector<uvg_rtp::frame::rtp_frame *> frames_;
            std::mutex frames_mtx_;
            std::mutex exit_mtx_;

            void *recv_hook_arg_;
            void (*recv_hook_)(void *arg, uvg_rtp::frame::rtp_frame *frame);
    };
}
