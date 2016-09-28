/* 
 **
 ** Use of source code is subject to the terms of the LeadCore license agreement under
 ** which you licensed source code. If you did not accept the terms of the license agreement,
 ** you are not authorized to use source code. For the terms of the license, please see the
 ** license agreement between you and LeadCore.
 **
 ** Copyright (c) 2013-2020  LeadCoreTech Corp.
 **
 **  PURPOSE:
 **
 **  CHANGE HISTORY:
 **
 **  Version		Date		Author		Description
 **  1.0.0		2013-06-05	zouqiao		created
 **
 */

#ifndef	__COMIP_NAND_H__
#define __COMIP_NAND_H__

/* Nand operation flags. */
#define NANDF_OOB_YAFFS			(0x00000001)
#define NANDF_OOB_RAW			(0x00000002)
#define NANDF_IGNORE_ERR		(0x00000100)
#define NANDF_CHECK_BLANK		(0x00000200)

extern int nand_init(void);
extern int nand_finalize(void);
extern int nand_write(uchar* buf, ulong start, ulong size, ulong flags);
extern int nand_read(uchar *buf, ulong start, ulong size, ulong flags);
extern int nand_erase(ulong start, ulong len);
extern int nand_erase_force(void);

#endif /*__COMIP_NAND_H__*/
