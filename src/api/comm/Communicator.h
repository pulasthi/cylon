#ifndef TWISTERX_COMMUNICATOR_H
#define TWISTERX_COMMUNICATOR_H

#include "../config/Config.h"
#include <queue>
#include <mutex>
#include "mpi.h"
#include "Buffer.h"
#include "messages/OutMessage.h"
#include "Receiver.h"

namespace twisterx::comm {
    class PendingRequest {
    private:
        int32_t src;
        Buffer *buffer;
        MPI_Request *request;
    public:
        PendingRequest(int32_t src, Buffer *buffer, MPI_Request *request) {
            this->src = src;
            this->buffer = buffer;
            this->request = request;
        }

        bool test() {
            MPI_Status status;
            int32_t flag = 0;
            MPI_Test(this->request, &flag, &status);
            //todo test properly
            return flag;
        }

        int32_t get_src() {
            return this->src;
        }

        Buffer *get_buffer() {
            return this->buffer;
        }
    };

    class Communicator {

    private:
        int32_t worker_id;

        int32_t op_id;

        std::queue<Buffer *> sending_buffers;
        std::queue<Buffer *> receiving_buffers;

        //<destination, queue>
        //one message per destination, initially
        std::map<int32_t, twisterx::comm::messages::Message *> out_messages;
        std::map<int32_t, PendingRequest *> receiving_requests;

        std::map<int32_t, Receiver *> receivers;

        std::mutex sending_buffers_lock;
        std::mutex receiving_requests_lock;
        std::mutex out_messages_lock;

        Buffer *burrow_sending() {
            Buffer *buffer = nullptr;
            sending_buffers_lock.lock();
            if (!sending_buffers.empty()) {
                buffer = sending_buffers.front();
                sending_buffers.pop();
            }
            sending_buffers_lock.unlock();
            return buffer;
        }

        void return_sending(Buffer *buffer) {
            sending_buffers_lock.lock();
            sending_buffers.push(buffer);
            sending_buffers_lock.unlock();
        }

        void post_recv(int32_t src, Buffer *buffer) {
            std::cout << "Post recv to : " << src << " from " << this->worker_id << std::endl;
            auto *mpi_request = new MPI_Request();
            MPI_Irecv(buffer->get_buffer(), buffer->size(), MPI_UNSIGNED_CHAR,
                      src, src, MPI_COMM_WORLD,
                      mpi_request);
            auto pending_request = new PendingRequest(src, buffer, mpi_request);
            this->receiving_requests.insert(std::pair<int32_t, PendingRequest *>(src, pending_request));
        }

    public:
        Communicator(twisterx::config::Config config, int32_t worker_id, int32_t world_size) {
            this->worker_id = worker_id;
            size_t buffer_size = 1024; //todo read from config
            for (int32_t i = 0; i < world_size; i++) { //todo read 4 from config
                sending_buffers.push(new Buffer(buffer_size));
                //receiving_buffers.push(new Buffer(buffer_size));
                if (i != worker_id && i < world_size) {
                    this->post_recv(i, new Buffer(buffer_size));
                }
            }
        }

        ~Communicator() {
            while (!sending_buffers.empty()) {
                delete sending_buffers.front();
                sending_buffers.pop();
            }
        }

        int32_t next_op_id() {
            return this->op_id++;
        }

        int32_t get_worker_id() {
            return this->worker_id;
        }

        void register_receiver(int32_t op_id, Receiver *receiver) {
            this->receivers.insert(std::pair<int32_t, Receiver *>(op_id, receiver));
        }

        template<class T>
        bool send_message(T *data, size_t size, int32_t destination, int32_t op_id) {
            bool accepted = false;
            out_messages_lock.lock();
            if (!this->out_messages.count(destination)) {
                this->out_messages[destination] = new twisterx::comm::messages::OutMessage<T>(data, size, destination,
                                                                                              op_id);

                this->out_messages[destination]->get_tag();
                accepted = true;
            }
            out_messages_lock.unlock();
            return accepted;
        }

        void print(std::string msg) {
            std::cout << msg << std::endl;
        }

        void progress() {
            out_messages_lock.lock();
            std::map<int32_t, twisterx::comm::messages::Message *>::iterator it;
            for (it = this->out_messages.begin(); it != this->out_messages.end(); it++) {
                if (it->second != nullptr) {
                    Buffer *send_buffer = burrow_sending();
                    if (send_buffer != nullptr) {
                        bool completed = it->second->offer_buffer(send_buffer);
                        int32_t sending_tag = 0;
                        if (completed) {
                            sending_tag = it->second->get_tag();
                            this->out_messages.erase(it);
                        }

                        MPI_Request mpiRequest;

                        //send to the destination
                        MPI_Isend(send_buffer->get_buffer(), send_buffer->position(),
                                  MPI_UNSIGNED_CHAR, sending_tag,
                                  this->worker_id, MPI_COMM_WORLD,
                                  &mpiRequest);

                        //todo need to Test mpiRequest
                    }
                }
            }
            out_messages_lock.unlock();

            receiving_requests_lock.lock();

            std::map<int32_t, PendingRequest *>::iterator recv_it;
            for (recv_it = this->receiving_requests.begin(); recv_it != this->receiving_requests.end(); recv_it++) {
                if (recv_it->second->test()) {
                    int32_t edge = recv_it->second->get_buffer()->get_int32();
                    std::cout << "Received for edge : " << edge << std::endl;

                    // call the receiver
                    this->receivers[edge]->receive(recv_it->second->get_src(), recv_it->second->get_buffer());

                    recv_it->second->get_buffer()->clear();

                    this->post_recv(recv_it->first, recv_it->second->get_buffer());

                    this->receiving_requests.erase(recv_it);
                }
            }

            receiving_requests_lock.unlock();
        }
    };
}

#endif //TWISTERX_COMMUNICATOR_H