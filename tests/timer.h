#include <chrono>
#include <map>

namespace epltree
{
  class Timer
  {
  public:
    void Record(std::string name)
    {
      time_map_.emplace(name, std::chrono::high_resolution_clock::now());
    }

    uint64_t Microsecond(std::string stop, std::string start)
    {
      return std::chrono::duration_cast<std::chrono::microseconds>(time_map_.at(stop) - time_map_.at(start)).count();
    }

    uint64_t Second(std::string stop, std::string start)
    {
      return std::chrono::duration_cast<std::chrono::seconds>(time_map_.at(stop) - time_map_.at(start)).count();
    }

    uint64_t Milliseconds(std::string stop, std::string start)
    {
      return std::chrono::duration_cast<std::chrono::milliseconds>(time_map_.at(stop) - time_map_.at(start)).count();
    }

    void Clear()
    {
      time_map_.clear();
    }

  private:
    std::map<std::string, std::chrono::_V2::high_resolution_clock::time_point> time_map_;
  };

  class TaskTimer
  {
  public:
    TaskTimer()
    {
    }

    TaskTimer(const Timer &timer) = delete;

    ~TaskTimer()
    {
      stop();
    }

    void start(int interval, std::function<void()> task)
    {
      if (!m_stop)
        return;
      m_stop = false;

      m_future = std::async(std::launch::async, [this, interval, task]()
                            {
            while (!m_stop)
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                const auto status = m_cv.wait_for(lock, std::chrono::milliseconds(interval));
                if(status == std::cv_status::timeout)
                    task();
            } });
    }

    void stop()
    {
      if (m_stop)
        return;

      {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_stop = true;
      }
      m_cv.notify_one();
      m_future.wait();
    }

  private:
    bool m_stop{true};
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::future<void> m_future;
  };

}