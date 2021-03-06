/*
 * kernel.h
 *
 *  Created on: Mar 26, 2015
 *      Author: lidq
 */

#ifndef _INCLUDE_KERNEL_KERNEL_H_
#define _INCLUDE_KERNEL_KERNEL_H_

#include <kernel/typedef.h>
#include <kernel/config.h>
#include <kernel/osiso.h>
#include <kernel/alloc.h>
#include <kernel/page.h>
#include <kernel/mm.h>
#include <kernel/process.h>
#include <kernel/page.h>
#include <kernel/tty.h>

int start_kernel(int argc, char **args);

void install_kernel();

void kernel_running();

#endif /* INCLUDE_KERNEL_KERNEL_H_ */
