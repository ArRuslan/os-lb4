#include <cstdint>
#include <iostream>

#include "lb4mailbox.h"

void printMb(MailBox* mb) {
    printf("max size: %d\n", mb->getMaxSize());
    printf("current size: %llu\n", mb->getCurrentSize());
    printf("entries count: %d\n", mb->getEntriesCount());
    printf("-------------------------------\n");
}

int main() {
    auto* mb = new MailBox("test.mb", 8);
    printMb(mb);

    mb->addEntry(new MailboxEntry("test 123"));
    mb->addEntry(new MailboxEntry("test"));
    printMb(mb);

    mb->deleteEntry(0);
    printMb(mb);

    return 0;
}
