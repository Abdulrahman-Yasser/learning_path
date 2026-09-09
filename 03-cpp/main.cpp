#include <stdlib.h>
#include <mutex>
#include <thread>
#include <iostream>
#include <stdio.h>
#include <queue>
#include <condition_variable>
using namespace std;


template <typename T>
class Cell{
    private:
        std::mutex my_mutex_;
        std::condition_variable cv_;
        // bool has_value = false;
        std::queue<T> my_queue;
        size_t capacity;
        bool _stop = false;
    public:
        explicit Cell(size_t capacity);
        void push(T value);
        bool pop(T& val);
        void stop();
};


template <typename T>
Cell<T>::Cell(size_t capacity):
    capacity(capacity)
{
}

template <typename T>
void Cell<T>::push(T value){
    std::unique_lock<std::mutex> lock(my_mutex_);
    this->cv_.wait(lock, [this]{ return my_queue.size() < capacity; });
    this->my_queue.push(value);
    this->cv_.notify_one();
}

template <typename T>
bool Cell<T>::pop(T& val){
    std::unique_lock<std::mutex> lock(my_mutex_);
    this->cv_.wait(lock, [this]{ return !my_queue.empty() || _stop; });
    if(my_queue.empty()){
        return false;
    }
    val = this->my_queue.front();
    this->my_queue.pop();
    this->cv_.notify_one();
    return true;
}

template <typename T>
void Cell<T>::stop(){
    {std::lock_guard<std::mutex> lock(my_mutex_); _stop = true;}
    cv_.notify_all();
}


class WarmupLatch{
    private:
        std::mutex my_mutex_;
        std::condition_variable cv_;
        bool done_ = false;
    public:
        void signal(){
            {
                std::lock_guard<std::mutex> lock(my_mutex_);
                done_ = true;
            }
            cv_.notify_all();
        }
        void wait(){
            std::unique_lock<std::mutex> lock(my_mutex_);
            cv_.wait(lock, [this]{return done_;});
        }
};



void camera_thread(Cell<int>& v, WarmupLatch& latch) {
    for(int frame_num =1; frame_num <= 100; frame_num++){
        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 10));
        if(frame_num <= 30){
            if(frame_num == 30){
                latch.signal();
            }
        }else{
            v.push(frame_num);
        }
    }
    v.stop();
}

void main_thread(Cell<int>& v, WarmupLatch& latch, int &received){
    latch.wait();
    int x, count = 0;
    while(v.pop(x)){
        count++;
    }
    received = count;
}

int main(){
    const int trials = 300;
    for (int t = 0; t < trials; t++) {
        Cell<int> my_val(5);
        WarmupLatch my_cam;
        int received = 0;

        std::thread th_1(camera_thread, std::ref(my_val), std::ref(my_cam));
        std::thread th_2(main_thread, std::ref(my_val), std::ref(my_cam), std::ref(received));
        th_1.join();
        th_2.join();

        if (received != 70) {
            std::cerr << "FAILED on trial " << t << ": got " << received << "\n";
            return 1;
        }
    }
    std::cout << trials << " trials, all passed\n";
}