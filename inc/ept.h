#ifndef JOS_INC_EPT_H
#define JOS_INC_EPT_H

#define __EPTE_READ	0x01
#define __EPTE_WRITE	0x02
#define __EPTE_EXEC	0x04
#define __EPTE_IPAT	0x40
#define __EPTE_SZ	0x80
#define __EPTE_A	0x100
#define __EPTE_D	0x200
#define __EPTE_TYPE(n)	(((n) & 0x7) << 3)

enum {
    EPTE_TYPE_UC = 0, /* uncachable */
    EPTE_TYPE_WC = 1, /* write combining */
    EPTE_TYPE_WT = 4, /* write through */
    EPTE_TYPE_WP = 5, /* write protected */
    EPTE_TYPE_WB = 6, /* write back */
};

#define __EPTE_NONE	0
#define __EPTE_FULL	(__EPTE_READ | __EPTE_WRITE | __EPTE_EXEC)

#endif
