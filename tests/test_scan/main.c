#include <stdio.h>
#include <assert.h>
#include "../../utils/wifi_scan.h"

int main(void) {
    wireless_scan_head head;
    wireless_scan *presult = NULL;

    memset(&head, 0, sizeof(head));
    assert(0 == wifi_scan("wlan0", &head));

    /* Traverse the results */
    presult = head.result;
    while (NULL != presult) {

        print_ap_info(presult);
        presult = presult->next;
    }

    return 0;
}
