/******************************************************************************
 * Copyright (c) 2019 Jason T. Harris. (sirmanlypowers@gmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef GGM_SRC_MODULE_SEQ_SEQ_H
#define GGM_SRC_MODULE_SEQ_SEQ_H

/******************************************************************************
 * constants
 */

#define MinBeatsPerMin (35.f)
#define MaxBeatsPerMin (350.f)

/******************************************************************************
 * sequencer op codes
 */

enum {
	SEQ_OP_NOP,		/* no operation */
	SEQ_OP_LOOP,		/* return to beginning */
	SEQ_OP_NOTE,		/* note on/off */
	SEQ_OP_REST,		/* rest */
	SEQ_OP_NUM		/* must be last */
};

/******************************************************************************
 * sequencer control values
 */

enum {
	SEQ_CTRL_STOP,		/* stop the sequencer */
	SEQ_CTRL_START,		/* start the sequencer */
	SEQ_CTRL_RESET,		/* reset the sequencer */
};

/*****************************************************************************/

#endif				/* GGM_SRC_MODULE_SEQ_SEQ_H */

/*****************************************************************************/
