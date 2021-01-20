#pragma once

#define RG_PRINT_TO_VECTOR(buf, format) do { \
        if (buf.empty()) buf.resize(1); \
        va_list args; \
        int n; \
        va_start(args, format); n = vsnprintf(buf.data(), buf.size(), format, args); va_end(args); \
        if (n < 0) return ""; \
        if ((size_t)(n + 1) > buf.size()) { \
            buf.resize((size_t)n + 1); \
            va_start(args, format); n = vsnprintf(buf.data(), buf.size(), format, args); va_end(args); \
        } \
        RG_ASSERT((size_t)n < buf.size()); \
        buf[n] = 0; \
    } while(0)
