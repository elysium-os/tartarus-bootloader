#pragma once

extern bool g_x86_64_cpu_nx_support;
extern bool g_x86_64_cpu_lapic_support;
extern bool g_x86_64_cpu_pdpe1gb_support;

void x86_64_cpu_init();
