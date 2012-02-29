#if !defined(rc_ixfile_h)
#define rc_ixfile_h

#include <stdint.h>
#include <vector>
#include <string>

#define MAX_FILE_SIZE (16*1024*1024)

class IxRead
{
public:
    ~IxRead();
    static IxRead *readFromFile(char const *name);
    void read(void *data, size_t size);
    void read_at(void *data, uint32_t size, uint32_t pos);
    uint32_t size() const;
    uint32_t pos() const;
    void seek(uint32_t pos);
    std::string const &type() const;
    void const *dataSegment(uint32_t from, uint32_t size) const;
private:
    IxRead();
    void *data_;
    size_t size_;
    size_t pos_;
    std::string type_;
};

class IxWrite
{
public:
    ~IxWrite();
    static IxWrite *writeToFile(char const *name);
    void write(void const *data, uint32_t size);
    void write_at(void const *data, uint32_t size, uint32_t pos);
    uint32_t size() const;
    uint32_t pos() const;
    void seek(uint32_t pos);
private:
    IxWrite();
    void *handle_;
    size_t pos_;
    size_t size_;
};

#endif  rc_ixfile_h
