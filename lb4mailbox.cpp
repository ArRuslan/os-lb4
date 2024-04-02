#include "lb4mailbox.h"

#include <cstring>
#include <fstream>
#include <filesystem>

char UINT_RW_ARR[4];

uint32_t crc32(const char* buf, uint32_t size) {
    uint32_t crc = 0xFFFFFFFF;
    while (size--) {
        crc ^= *buf++;
        for (int k = 0; k < 8; k++)
            crc = crc & 1 ? crc >> 1 ^ 0x82f63b78 : crc >> 1;
    }
    return ~crc;
}

uint32_t readUint32(std::fstream& file) {
    uint32_t result;
    file.read(UINT_RW_ARR, 4);
    memcpy(&result, UINT_RW_ARR, 4);

    return result;
}

uint32_t readUint32(std::ifstream& file) {
    return readUint32((std::fstream&)file);
}


MailboxEntry::MailboxEntry(const char* content, const uint32_t size) {
    this->content_size = content[size-1] == '\0' ? size - 1 : size;
    this->content = new char[size];
    memcpy(this->content, content, size);

    this->checksum = crc32(this->content, this->content_size);
}

MailboxEntry::MailboxEntry(const std::string& content) {
    this->content_size = content.size();
    this->content = new char[this->content_size];
    memcpy(this->content, content.c_str(), this->content_size);

    this->checksum = crc32(this->content, this->content_size);
}

std::string MailboxEntry::getContent() {
    return std::string(content);
}


void MailboxEntry::write(std::ofstream& file) {
    memcpy(UINT_RW_ARR, &content_size, 4);
    file.write(UINT_RW_ARR, 4);

    memcpy(UINT_RW_ARR, &checksum, 4);
    file.write(UINT_RW_ARR, 4);

    file.write(content, content_size);
}

MailBox::MailBox(const std::string& name) {
    filename = name;

    std::ifstream file(name, std::ios::binary);
    max_size = readUint32(file);
    current_index = readUint32(file);

    file.seekg(0, std::ios::end);
    const long size = file.tellg();
    file.close();
    if(size < max_size + 8)
        throw std::underflow_error("Size if mailbox is too small!");
}

MailBox::MailBox(const std::string& name, const uint32_t max_size) {
    filename = name;
    this->max_size = max_size;

    std::ofstream file(name, std::ios::binary | std::ios::trunc);
    memcpy(UINT_RW_ARR, &max_size, 4);
    file.write(UINT_RW_ARR, 4);
    memcpy(UINT_RW_ARR, &current_index, 4);
    file.write(UINT_RW_ARR, 4);

    uint32_t tmp = 0;
    for(int i = 0; i < max_size; i++) {
        memcpy(UINT_RW_ARR, &tmp, 4);
        file.write(UINT_RW_ARR, 4);
    }

    file.close();
}

uint32_t MailBox::getMaxSize() {
    return max_size;
}

uint32_t MailBox::getEntriesCount() {
    return current_index;
}

uint64_t MailBox::getCurrentSize() {
    std::ifstream file(filename, std::ios::binary);
    uint64_t total_size = 0;

    file.seekg(8);

    for(uint32_t i = 0; i < current_index; i++) {
        uint32_t tmp = readUint32(file);

        uint32_t current_pos = file.tellg();
        file.seekg(max_size*4+8+tmp);

        total_size += readUint32(file);

        file.seekg(current_pos);
    }

    file.close();

    return total_size;
}

void MailBox::addEntry(MailboxEntry* entry) {
    if(current_index >= max_size)
        throw std::overflow_error("Mailbox is full!");

    std::ofstream file(filename, std::ios::ate | std::ios::in | std::ios::out | std::ios::binary);

    const uint32_t ptr = (long)file.tellp() - (max_size*4+8);
    entry->write(file);

    file.seekp(current_index*4+8);
    memcpy(UINT_RW_ARR, &ptr, 4);
    file.write(UINT_RW_ARR, 4);

    current_index++;

    file.seekp(4);
    memcpy(UINT_RW_ARR, &current_index, 4);
    file.write(UINT_RW_ARR, 4);

    file.close();
}

MailboxEntry* MailBox::readEntry(const uint32_t index, const bool del) {
    if(index >= current_index)
        throw std::range_error("Requested mail entry does not exist!");

    std::ifstream file(filename, std::ios::binary);

    file.seekg(8+(index*4));
    uint32_t tmp = readUint32(file);

    file.seekg(max_size*4+8+tmp);

    uint32_t size = readUint32(file);
    uint32_t checksum = readUint32(file);

    char* content = new char[size+1];
    file.read(content, size);
    content[size] = '\0';
    file.close();

    if(crc32(content, size) != checksum)
        throw std::runtime_error("Mail entry checksum mismatch!");

    auto* entry = new MailboxEntry(content, size+1);

    delete[] content;

    if(del)
        deleteEntry(index);

    return entry;
}

void MailBox::deleteEntry(uint32_t index) {
    if(index >= current_index)
        throw std::range_error("Requested mail entry does not exist!");
    std::fstream file(filename, std::ios::ate | std::ios::in | std::ios::out | std::ios::binary);

    file.seekg(8+(index*4));
    uint32_t tmp = readUint32(file);
    file.seekg(8+(max_size*4) + tmp);
    tmp = readUint32(file);
    uint32_t bytes_to_move = tmp + 8;

    uint32_t indexes_to_move = current_index - index - 1;
    char* indexes = new char[indexes_to_move * 4];

    file.seekg(8 + (4 * index)+4);
    file.read(indexes, indexes_to_move * 4);

    file.seekp(8 + (4 * index));
    file.write(indexes, indexes_to_move * 4);

    delete[] indexes;

    current_index--;

    file.seekp(4);
    memcpy(UINT_RW_ARR, &current_index, 4);
    file.write(UINT_RW_ARR, 4);

    file.seekg(8);
    for(uint32_t i = index; i < current_index; i++) {
        file.seekg(8+(i*4));
        file.seekg(8+(max_size*4) + readUint32(file));
        uint32_t size_to_move = readUint32(file) + 8;

        char* to_move = new char[size_to_move];

        file.seekg(-4, std::ios::cur);
        long cur_pos = file.tellg();
        file.read(to_move, size_to_move);

        file.seekp(cur_pos - bytes_to_move);
        file.write(to_move, size_to_move);

        delete[] to_move;

        file.seekg(8+(i*4));

        tmp = readUint32(file);
        tmp -= bytes_to_move;

        file.seekp(8+(i*4));
        memcpy(UINT_RW_ARR, &tmp, 4);
        file.write(UINT_RW_ARR, 4);
    }

    file.seekg(0, std::ios::end);
    uint32_t file_size = file.tellg();

    file.close();

    std::filesystem::resize_file(filename, file_size-bytes_to_move);
}
