#!/bin/ash
while [ 1 ]; do
        SWs=$(devmem 0x41200000 b)
        BTNs=$(devmem 0x41240000 b)

        #ONE_BTN_PRESSED=$(($(($BTNs-1))&$BTNs)
        #echo $ONE_BTN_PRESSED

        if [ "$BTNs" = "0x00" ]; then
                $(devmem 0x41220000 b $SWs)
        elif [ $(($((BTNs-1))&$BTNs)) -eq 0 ]; then
                $(devmem 0x41220000 b $((0xFF-$SWs)))
        else
                $(devmem 0x41220000 b 0xFF)
        fi
done
