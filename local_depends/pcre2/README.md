https://github.com/PCRE2Project/pcre2.git

cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DPCRE2_SUPPORT_JIT=ON -B build

cmake --build build/
