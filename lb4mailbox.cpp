#include "lb4mailbox.h"

#include <cstring>
#include <fstream>

uint32_t crc32(const char* buf, uint32_t size) {
    uint32_t crc = 0xFFFFFFFF;
    while (size--) {
        crc ^= *buf++;
        for (int k = 0; k < 8; k++)
            crc = crc & 1 ? crc >> 1 ^ 0x82f63b78 : crc >> 1;
    }
    return ~crc;
}

MailboxEntry::MailboxEntry(const char* content, const uint32_t size) {
    this->content_size = size;
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

void MailboxEntry::write(std::ofstream& file) {
    const auto buf = new char[4];
    memcpy(buf, &content_size, 4);
    file.write(buf, 4);
    memcpy(buf, &checksum, 4);
    file.write(buf, 4);
    delete[] buf;
    file.write(content, content_size);
}

MailBox::MailBox(const std::string& name) {
    filename = name;

    std::ifstream file(name, std::ios::binary);
    const auto buf = new char[4];
    file.read(buf, 4);
    memcpy(&max_size, buf, 4);
    file.read(buf, 4);
    memcpy(&current_index, buf, 4);

    delete[] buf;

    file.seekg(0, std::ios::end);
    const long size = file.tellg();
    file.close();
    if(size < max_size + 8)
        throw std::underflow_error("Size if mailbox is too small!");
}

MailBox::MailBox(const std::string& name, const uint32_t max_size) {
    filename = name;

    std::ofstream file(name, std::ios::binary | std::ios::trunc);
    const auto buf = new char[4];
    memcpy(buf, &max_size, 4);
    file.write(buf, 4);
    memcpy(buf, &current_index, 4);
    file.write(buf, 4);

    for(int i = 0; i < max_size; i++)
        file.write(buf, 4);

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

    const auto buf = new char[4];
    uint32_t tmp;
    for(uint32_t i = 0; i < current_index; i++) {
        file.read(buf, 4);
        memcpy(&tmp, buf, 4);

        uint32_t current_pos = file.tellg();
        file.seekg(max_size*4+8+tmp);

        file.read(buf, 4);
        memcpy(&tmp, buf, 4);
        total_size += tmp;

        file.seekg(current_pos);
    }

    delete[] buf;
    file.close();

    return total_size;
}

void MailBox::addEntry(MailboxEntry* entry) {
    if(current_index >= max_size)
        throw std::overflow_error("Mailbox is full!");

    std::ofstream file(filename, std::ios::ate | std::ios::in | std::ios::out | std::ios::binary);

    const uint32_t ptr = file.tellp();
    entry->write(file);

    const auto buf = new char[4];

    file.seekp(current_index*4+8);
    memcpy(buf, &ptr, 4);
    file.write(buf, 4);

    current_index++;

    file.seekp(4);
    memcpy(buf, &current_index, 4);
    file.write(buf, 4);

    file.close();
}

MailboxEntry* MailBox::readEntry(const uint32_t index, const bool del) {
    if(index >= current_index)
        throw std::range_error("Requested mail entry does not exist!");

    std::ifstream file(filename, std::ios::binary);
    const auto buf = new char[4];
    uint32_t tmp;

    file.seekg(8+(index*4));
    file.read(buf, 4);
    memcpy(&tmp, buf, 4);

    file.seekg(max_size*4+8+tmp);

    uint32_t size;
    file.read(buf, 4);
    memcpy(&size, buf, 4);

    uint32_t checksum;
    file.read(buf, 4);
    memcpy(&checksum, buf, 4);

    file.close();

    const char* content = new char[size];
    if(crc32(content, size) != checksum)
        throw std::runtime_error("Mail entry checksum mismatch!");

    auto* entry = new MailboxEntry(content, size);

    delete[] buf;
    delete[] content;

    if(del)
        deleteEntry(index);

    return entry;
}

void MailBox::deleteEntry(uint32_t index) {
    //

    rebuildStructure();
}

void MailBox::rebuildStructure() {
    //
}
