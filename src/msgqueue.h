/*
   Thread-save message queue used for inter-process communication.
   One task can push data into the queue. Another thread is waiting for the data to pop them out of the queue.
*/

#ifndef MSGQUEUE_H
#define MSGQUEUE_H

#include <mutex>
#include <condition_variable>


template <typename T, unsigned int N>
class MessageQueue
{
public:
   MessageQueue()
   {
      _count = 0;
      write = 0;
      read = 0;
   }

   bool full() const
   {
      return (_count > N);
   }

   bool empty() const
   {
      return (_count == 0);
   }

   unsigned int count() const
   {
      return _count;
   }


   //if queue is full function returns immediatly with false -> message is discarded
   //return true on success
   bool push_nowait(const T& msg)
   {
      bool status = false;
      std::unique_lock<std::mutex> lock(_mutex); //mutex is locked on construction; mutex is release on destruction
      if (_count < N) //space left
      {
         ++_count;
         queue[write++] = msg; //copy message into queue
         if (write >= N) write = 0; //wrap around
         _condvar.notify_one(); //inform waiting thread that a new message is available
         status = true; //success
      }
      return status;
   }

   //if queue is empty thread is suspended until a message becomes available
   void pop(T& msg)
   {
      std::unique_lock<std::mutex> lock(_mutex); //mutex is locked on construction; mutex is release on destruction
      do
      {
         if (_count > 0) //message available
         {
            --_count;
            msg = queue[read++]; //get message from queue
            if (read >= N) read = 0; //wrap around
            break; //done
         }
         //no message available
         //wait on condition variable for notification and try again
         _condvar.wait(lock); //wait operation releases the locked mutex (and aquires it when finished waiting...)
      } while (true);
   }

   //if queue is empty function returns immediatly with false
   //return true on success
   bool pop_nowait(T& msg)
   {
      bool status = false;
      std::unique_lock<std::mutex> lock(_mutex); //mutex is locked on construction; mutex is release on destruction
      if (_count > 0) //message available
      {
         --_count;
         msg = queue[read++]; //get message from queue
         if (read >= N) read = 0; //wrap around
         status = true; //success
      }
      return status;
   }


private:
   T queue[N];
   unsigned int _count;
   unsigned int write;
   unsigned int read;
   std::mutex _mutex;
   std::condition_variable _condvar;
};


#endif
