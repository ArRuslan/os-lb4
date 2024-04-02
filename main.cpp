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

    mb->addEntry(new MailboxEntry("test 1"));
    mb->addEntry(new MailboxEntry("test 2"));

    mb->deleteEntry(1);
    printMb(mb);

    for(int i = 0; i < mb->getEntriesCount(); i++) {
        auto* entry = mb->readEntry(i);
        printf("Entry #%d:\n  %s\n", i, entry->getContent().c_str());
        delete entry;
    }

    return 0;
}
