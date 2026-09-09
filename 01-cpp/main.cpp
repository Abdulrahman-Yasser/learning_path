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
    // if(my_queue.size() == this->capacity-1){
    //     cout << "FULL" << endl;
    //     return;
    // }
    this->cv_.wait(lock, [this]{ return my_queue.size() < capacity; });
    this->my_queue.push(value);
    // has_value = true;
    this->cv_.notify_one();
}

template <typename T>
bool Cell<T>::pop(T& val){
    std::unique_lock<std::mutex> lock(my_mutex_);
    this->cv_.wait(lock, [this]{ return !my_queue.empty() || _stop; });
    // has_value = false;
    // if(my_queue.size() == 0){
    //     cout << "EMPTY" << endl;
    //     return false;
    // }
    val = this->my_queue.front();
    this->my_queue.pop();
    return true;
}

template <typename T>
void Cell<T>::stop(){
    {std::lock_guard<std::mutex> lock(my_mutex_); _stop = true;}
    cv_.notify_all();
}


void thread_1(Cell<int>& v) { 
    for (int i = 0; i < 100000; i++){
        int v_t;
        v.pop(v_t);
        v.push(v_t+1);
    }
    int r;
    v.pop(r);
    v.push(r);
    cout << "Last value th1 is " << r << endl;
}
void thread_2(Cell<int>& v) { 
    for (int i = 0; i < 100000; i++){
        int v_t;
        v.pop(v_t);
        v.push(v_t+1);
    }
    int r;
    v.pop(r);
    v.push(r);
    cout << "Last value th2 is " << r << endl;
}

int main(){
    Cell<int> my_val(5);
    my_val.push(0);
    std::thread th_1(thread_1, std::ref(my_val)), th_2(thread_2, std::ref(my_val));
    th_1.join();
    th_2.join();
    // while(1);
}
