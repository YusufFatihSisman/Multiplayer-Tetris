#ifndef TSQUEUE_H
#define TSQUEUE_H

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable> 

template<typename T>
class tsqueue{
    std::queue<T> queue;
    std::mutex mtx;

    std::condition_variable cv; 

    public:
        tsqueue() = default;
		tsqueue(const tsqueue<T>&) = delete;

        T pop(){
            std::scoped_lock<std::mutex> lock(mtx);
            T front = queue.front();
            queue.pop();
            return front;
        }

        T front(){
            std::scoped_lock<std::mutex> lock(mtx);
            return queue.front();
        }

        bool empty(){
            std::scoped_lock <std::mutex> lock(mtx);
            return queue.empty();
        }

        void push(const T& newElement){
            std::scoped_lock <std::mutex> lock(mtx);
            queue.push(std::move(newElement));
            cv.notify_one();
        }
    
        void wait(){
            std::unique_lock<std::mutex> condLock(mtx);
            while(condEmpty()){
                cv.wait(condLock);
            }
        }

    private:
        bool condEmpty(){
            return queue.empty();
        }
};

#endif