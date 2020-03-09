/** A wrapper for h5cpp calls
 */

#ifndef WIRECELLHIO_UTIL
#define WIRECELLHIO_UTIL

#include <mutex>
#include <thread>
#include <chrono>

#include <h5cpp/all>

namespace WireCell {
namespace Hio {

extern std::mutex g_h5cpp_mutex;
// static std::mutex g_h5cpp_mutex;

///
inline h5::fd_t create(const std::string &path, unsigned flags,
                const h5::fcpl_t &fcpl = h5::default_fcpl,
                const h5::fapl_t &fapl = h5::default_fapl) {
  std::lock_guard<std::mutex> guard(g_h5cpp_mutex);
  return h5::create(path, flags, fcpl, fapl);
}
///
template <class T, class... args_t>
void write(const h5::fd_t &fd, const std::string &dataset_path,
               const T *ptr, args_t &&... args) {
  std::lock_guard<std::mutex> guard(g_h5cpp_mutex);
  std::cout << "[yuhw]: Hio::write : START ... ";
  try {
    h5::write(fd, dataset_path, ptr, args...);
  } catch (...) {
    std::cout << "h5::write " << dataset_path << " failed! ";
  }
  // std::this_thread::sleep_for(std::chrono::milliseconds(100));
  std::cout << "END\n";
};

///
inline h5::fd_t open(const std::string &path, unsigned flags,
              const h5::fapl_t &fapl = h5::default_fapl) {
  std::lock_guard<std::mutex> guard(g_h5cpp_mutex);
  return h5::open(path, flags, fapl);
}

/// read: pointer
template <class T, class... args_t>
void read(const h5::fd_t &fd, const std::string &dataset_path, T *ptr,
          args_t &&... args) {
  std::lock_guard<std::mutex> guard(g_h5cpp_mutex);
  h5::read(fd, dataset_path, ptr, args...);
}

/// read: object
template <class T, class... args_t>
T read(const h5::fd_t &fd, const std::string &dataset_path, args_t &&... args) {
  std::lock_guard<std::mutex> guard(g_h5cpp_mutex);
  std::cout << "[yuhw]: Hio::read : START ... ";
  auto t = h5::read<T>(fd, dataset_path, args...);
  std::cout << "END\n";
  return t;
}
}; // namespace Hio
} // namespace WireCell

#endif // WIRECELLHIO_UTIL