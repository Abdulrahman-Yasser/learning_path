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
int count = 0;

void thread_1(Cell<int>& v) { 
    for (int i = 0; i < 100000; i++){
        v.push(i);
    }
    cout << "Last value th1 is " << endl;
}
void thread_2(Cell<int>& v) { 
    for (int i = 0; i < 100000; i++){
        v.push(i);
    }
    cout << "Last value th2 is " << endl;
}
void thread_3(Cell<int>& v) { 
    for (int i = 0; i < 100000; i++){
        v.push(i);
    }
    cout << "Last value th3 is " << endl;
}
void thread_4(Cell<int>& v) { 
    int local_count = 0;
    int x;
    while (v.pop(x)){
        cout << "value th4 is " << x << endl;
        local_count++;
    }
    cout << "Last value th4 is " << local_count << endl;
}
void thread_5(Cell<int>& v) { 
    int local_count = 0;
    int x;
    while (v.pop(x)){
        cout << "value th5 is " << x << endl;
        local_count++;
    }
    cout << "Last value th5 is " << local_count << endl;
}

int main(){
    Cell<int> my_val(5);
    my_val.push(0);
    std::thread th_1(thread_1, std::ref(my_val)), th_2(thread_2, std::ref(my_val)), th_3(thread_3, std::ref(my_val)), th_4(thread_4, std::ref(my_val)), th_5(thread_5, std::ref(my_val));
    th_1.join();
    th_2.join();
    th_3.join();
    my_val.stop();
    th_4.join();
    th_5.join();
    // while(1);
}
