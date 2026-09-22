/**
 * @file singleton_test.cpp
 * @author Forairaaaaa
 * @brief
 * @version 0.1
 * @date 2024-09-30
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <cstdio>
#include <mooncake.h>

using namespace mooncake;

int main()
{
    printf("app name: %zu\n", GetMooncake().getAppNum());

    DestroyMooncake();
    GetMooncake();

    return 0;
}
