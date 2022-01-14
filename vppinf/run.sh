#!/usr/bin/env bash
echo "--->>> main"
./main
echo "--->>> bond"
./bond
echo "--->>> buffers"
./buffers
echo "--->>> ceq"
./ceq
echo "--->>> memcpyle"
./memcpyle
echo "--->>> memsetux"
./memsetux
echo "--->>> rdma"
./rdma FFFFFFF

