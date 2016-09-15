#!/bin/bash

cd ../make
echo "====================================================================="
echo "====================================================================="
echo -e "\t\t\t\tTEST CASE"
echo "====================================================================="
echo "====================================================================="
echo -e "\n1. An invalid option, exception: -f, -t, -v, -s, -e\n\n./rftu -u"
./rftu -h
echo "====================================================================="
echo -e "\n2. Running with lack of option\n\n./rftu -t"
./rftu -t
echo "====================================================================="
echo -e "\n3. Running with redundancy option\n\n./rftu -s -t"
./rftu -s -t
echo "====================================================================="
echo -e "\n4. Handling the file that does not exist\n\n./rftu -f /home/humax/f"
./rftu -f /home/humax/f
echo "====================================================================="
echo -e "\n5. Handling the file that does not exist\n\n./rftu -f /home/humax/file -t 290.0.0.1"
./rftu -f /home/humax/file -t 290.0.0.1
echo "====================================================================="
cd ../docs
exec bash
