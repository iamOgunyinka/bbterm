#include "screenbuffer.h"

#include <core/util/log.h>

#include <QStringList>
#include <QDebug>

//#define ESC_DEBUG() qDebug() << "[CTL]" << Q_FUNC_INFO
#define ESC_DEBUG() while(false) qDebug()
#define ESC_DEBUG_NIY() qWarning() << "[NIY CTL]" << Q_FUNC_INFO

using namespace core::term;

struct EscCommand
{
	QLatin1String pattern;
	void (ScreenBuffer::*handler)(const QStringList &);
	bool isRegexp;
};

static EscCommand esc_commands_vt100[] = {
	{QLatin1String("\x7"), &ScreenBuffer::escape_bel, false},
	{QLatin1String("^\x001b\\[([0-9]?)m"), &ScreenBuffer::escape_charAttr, true},
//	{QLatin1String("\x1b[0m"), &ScreenBuffer::escape_sgr0, false},
//	{QLatin1String("\x1b[1m"), &ScreenBuffer::escape_bold, false},
//	{QLatin1String("\x1b[4m"), &ScreenBuffer::escape_smul, false},
//	{QLatin1String("\x1b[5m"), &ScreenBuffer::escape_blink, false},
//	{QLatin1String("\x1b[7m"), &ScreenBuffer::escape_rev, false},
//	{QLatin1String("\x1b[m"), &ScreenBuffer::escape_rmso, false},
//	{QLatin1String("\x1b[m"), &ScreenBuffer::escape_rmul, false},
//	{QLatin1String("\x1b[7m"), &ScreenBuffer::escape_smso, false},
	{QLatin1String("\x1b[2J"), &ScreenBuffer::escape_clear, false},
	{QLatin1String("\xd"), &ScreenBuffer::escape_cr, false},
	{QLatin1String("^\x001b\\[([0-9]*);([0-9]*)r"), &ScreenBuffer::escape_csr, true},
	{QLatin1String("^\x001b\\[([0-9]*)D"), &ScreenBuffer::escape_cub, true},
	//{QLatin1String("\x8"), &ScreenBuffer::escape_cub1, false},
	{QLatin1String("^\x001b\\[([0-9]*)B"), &ScreenBuffer::escape_cud, true},
	//{QLatin1String("\xa"), &ScreenBuffer::escape_cud1, false},
	{QLatin1String("^\x001b\\[([0-9]*)C"), &ScreenBuffer::escape_cuf, true},
	//{QLatin1String("\x1b[C"), &ScreenBuffer::escape_cuf1, false},
	{QLatin1String("^\x001b\\[([0-9]+);([0-9]+)H"), &ScreenBuffer::escape_cup, true},
	{QLatin1String("^\x001b\\[([0-9]+)A"), &ScreenBuffer::escape_cuu, true},
	{QLatin1String("\x1b[A"), &ScreenBuffer::escape_cuu1, false},
	{QLatin1String("\x1b[J"), &ScreenBuffer::escape_ed, false},
	{QLatin1String("\x1b[K"), &ScreenBuffer::escape_el, false},
	{QLatin1String("\x1b[1K"), &ScreenBuffer::escape_el1, false},
	{QLatin1String("\x1b(B\x1b)0"), &ScreenBuffer::escape_enacs, false},
	{QLatin1String("\x1b[H"), &ScreenBuffer::escape_home, false},
	{QLatin1String("\x9"), &ScreenBuffer::escape_ht, false},
	{QLatin1String("\x1bH"), &ScreenBuffer::escape_hts, false},
	//{QLatin1String("\xa"), &ScreenBuffer::escape_ind, false},
	{QLatin1String("\x1bOq"), &ScreenBuffer::escape_ka1, false},
	{QLatin1String("\x1bOs"), &ScreenBuffer::escape_ka3, false},
	{QLatin1String("\x1bOr"), &ScreenBuffer::escape_kb2, false},
	{QLatin1String("\x8"), &ScreenBuffer::escape_kbs, false},
	{QLatin1String("\x1bOp"), &ScreenBuffer::escape_kc1, false},
	{QLatin1String("\x1bOn"), &ScreenBuffer::escape_kc3, false},
	{QLatin1String("\x1bOD"), &ScreenBuffer::escape_kcub1, false},
	{QLatin1String("\x1bOB"), &ScreenBuffer::escape_kcud1, false},
	{QLatin1String("\x1bOC"), &ScreenBuffer::escape_kcuf1, false},
	{QLatin1String("\x1bOA"), &ScreenBuffer::escape_kcuu1, false},
	{QLatin1String("\x1bOM"), &ScreenBuffer::escape_kent, false},
	{QLatin1String("\x1bOy"), &ScreenBuffer::escape_kf0, false},
	{QLatin1String("\x1bOP"), &ScreenBuffer::escape_kf1, false},
	{QLatin1String("\x1bOx"), &ScreenBuffer::escape_kf10, false},
	{QLatin1String("\x1bOQ"), &ScreenBuffer::escape_kf2, false},
	{QLatin1String("\x1bOR"), &ScreenBuffer::escape_kf3, false},
	{QLatin1String("\x1bOS"), &ScreenBuffer::escape_kf4, false},
	{QLatin1String("\x1bOt"), &ScreenBuffer::escape_kf5, false},
	{QLatin1String("\x1bOu"), &ScreenBuffer::escape_kf6, false},
	{QLatin1String("\x1bOv"), &ScreenBuffer::escape_kf7, false},
	{QLatin1String("\x1bOl"), &ScreenBuffer::escape_kf8, false},
	{QLatin1String("\x1bOw"), &ScreenBuffer::escape_kf9, false},
	// lf1
	// lf2
	// lf3
	// lf4
	{QLatin1String("\x1b[0i"), &ScreenBuffer::escape_mc0, false},
	{QLatin1String("\x1b[4i"), &ScreenBuffer::escape_mc4, false},
	{QLatin1String("\x1b[5i"), &ScreenBuffer::escape_mc5, false},
	{QLatin1String("\x1b""8"), &ScreenBuffer::escape_rc, false},
	{QLatin1String("\x1bM"), &ScreenBuffer::escape_ri, false},
	{QLatin1String("\xf"), &ScreenBuffer::escape_rmacs, false},
	{QLatin1String("\x1b[?7l"), &ScreenBuffer::escape_rmam, false},
	{QLatin1String("\x1b[?1l\x1b>"), &ScreenBuffer::escape_rmkx, false},
	// rs2
	{QLatin1String("\x1b""7"), &ScreenBuffer::escape_sc, false},
	// sgr
	{QLatin1String("\xe"), &ScreenBuffer::escape_smacs, false},
	{QLatin1String("\x1b[?7h"), &ScreenBuffer::escape_smam, false},
	{QLatin1String("\x1b[?1h\x1b"), &ScreenBuffer::escape_smkx, false},
	{QLatin1String("\x1b[3g"), &ScreenBuffer::escape_tbc, false},
};
static const int esc_commands_count_vt100 = sizeof(esc_commands_vt100)/sizeof(EscCommand);

// graphics charset pairs, based on vt100
void ScreenBuffer::escape_acsc(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
}

// audible signal (bell)
void ScreenBuffer::escape_bel(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
//  if( term->bell != NULL ) term->bell(TO_H(term));
	*/
}

// Clear screen and home cursor
void ScreenBuffer::escape_clear(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	int i, j;
	for( i = 0; i < term->grid.height; i ++ ) {
		for( j = 0; j < term->grid.width; j ++ ) {
			term->grid.grid[ i ][ j ] = ' ';
			term->grid.attribs[ i ][ j ] = 0;
		}
	}
	term->crow = 0;
	term->ccol = 0;
	term_add_dirty_rect( term, 0, 0, term->grid.width, term->grid.height );
	term->dirty_cursor.exists = true;
	*/
}

// carriage return
void ScreenBuffer::escape_cr(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG();
	m_currentPosition.setX(0);
	/*
//  term->ccol = 0;
	*/
}

// Change scrolling region
void ScreenBuffer::escape_csr(const QStringList &params)
{
	Q_UNUSED(params);
	int n1 = params.value(0).toInt();
	int n2 = params.value(1).toInt();
	ESC_DEBUG() << "Enable scrolling from row:" << n1 << "to row:" << n2 << "NIY";
}

// Move cursor left #1 spaces
void ScreenBuffer::escape_cub(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	int left = atoi( term->output_bytes + 2 );
	if( term->ccol < left ) {
		term->ccol = 0;
	}  else {
		term->ccol -= left;
	}
	term->dirty_cursor.exists = 1;
	*/
}

// Move down #1 lines
void ScreenBuffer::escape_cud(const QStringList &params)
{
	bool ok;
	int n = params.value(0).toInt(&ok);
	if(!ok)
		n = 1;
	ESC_DEBUG() << n << "lines feed";
	for(int i=0; i<n; i++)
		appendLine(true);
}

// Move right #1 spaces
// The CUF sequence moves the active position to the right. The distance moved is determined by the parameter. A parameter value of zero or one moves the active position one position to the right. A parameter value of n moves the active position n positions to the right. If an attempt is made to move the cursor to the right of the right margin, the cursor stops at the right margin.
void ScreenBuffer::escape_cuf(const QStringList &params)
{
	Q_UNUSED(params);
	int n = params.value(0).toInt();
	ESC_DEBUG() << "n:" << n;
	int x = m_currentPosition.x() + n;
	if(x >= m_terminalSize.width()) {
		x = m_terminalSize.width() - 1;
	}
	m_currentPosition.setX(x);
}

// Move to row #1 col #2
void ScreenBuffer::escape_cup(const QStringList &params)
{
	Q_UNUSED(params);
	int row = params.value(0).toInt() - 1;
	int col = params.value(1).toInt() - 1;
	ESC_DEBUG() << "row:" << row << "col:" << col;
	if(row >= 0 && row < m_terminalSize.height()) {
		if(col >= 0 && col < m_terminalSize.width()) {
			while(rowCount() <= row) {
				appendLine(false);
			}
			m_currentPosition.setX(col);
			m_currentPosition.setY(row);
		}
		else {
			LOGWARN() << "col:" << row << "out of range:" << m_terminalSize.width();
		}
	}
	else {
		LOGWARN() << "row:" << row << "out of range:" << m_terminalSize.height();
	}
}

// Move cursor up #1 lines
void ScreenBuffer::escape_cuu(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	int up = atoi( term->output_bytes + 2 );
	if( term->crow - up < 0 ) {
		term->crow = 0;
	}  else {
		term->crow -= up;
	}
	term->dirty_cursor.exists = true;
	*/
}

// up one line
void ScreenBuffer::escape_cuu1(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	if( term->crow > 0 ) {
		term->crow --;
		term->dirty_cursor.exists = true;
	}
	*/
}

// Clear to end of display
void ScreenBuffer::escape_ed(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG() << "Clear to end of display";
	QPoint pos = m_currentPosition;
	while(m_currentPosition.y() < m_terminalSize.height()) {
		escape_el(QStringList());
		m_currentPosition.setX(0);
		m_currentPosition.setY(m_currentPosition.y() + 1);
	}
	m_currentPosition = pos;
}

// Clear to end of line
void ScreenBuffer::escape_el(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG() << "Clear to end of line";
	int row = firstVisibleLineIndex() + m_currentPosition.x();
	if(row < rowCount()) {
		ScreenLine &line = m_lineBuffer.at(row);
		for(int i=m_currentPosition.x(); i<m_terminalSize.width() && i<line.length(); i++) {
			ScreenCell &cell = line.cellAt(i);
			cell.setLetter(QChar());
			cell.setColor(ScreenCell::ColorWhite, ScreenCell::ColorBlack);
			cell.setAttributes(ScreenCell::AttrReset);
		}
	}
}

// Clear from beginning of line to cursor
void ScreenBuffer::escape_el1(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG() << "Clear from beginning of line to cursor";
	int row = firstVisibleLineIndex() + m_currentPosition.x();
	if(row < rowCount()) {
		ScreenLine &line = m_lineBuffer.at(row);
		for(int i=0; i<=m_currentPosition.x(); i++) {
			ScreenCell &cell = line.cellAt(i);
			cell.setLetter(QChar());
			cell.setColor(ScreenCell::ColorWhite, ScreenCell::ColorBlack);
			cell.setAttributes(ScreenCell::AttrReset);
		}
	}
}

// enable alternate char set
void ScreenBuffer::escape_enacs(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG() << "enable alternate char set NIY";
}

// Home cursor
void ScreenBuffer::escape_home(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG() << "cursor home";
	m_currentPosition.setX(0);
	m_currentPosition.setY(0);
}

// turn on blank mode (characters invisible)
void ScreenBuffer::escape_invis(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
}

// tab to next 8-space hardware tab stop
void ScreenBuffer::escape_ht(const QStringList &params)
{
	Q_UNUSED(params);
	static const int tab_width = 8;
	int x = m_currentPosition.x();
	x = (x / tab_width + 1) * tab_width;
	m_currentPosition.setX(x);
	if(m_currentPosition.x() >= m_terminalSize.width()) {
		//appendLine();
	}
}

// set a tab in every row, current columns
void ScreenBuffer::escape_hts(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
}

// scroll text up
void ScreenBuffer::escape_ind(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
}

// upper left of keypad
void ScreenBuffer::escape_ka1(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
}

// upper right of keypad
void ScreenBuffer::escape_ka3(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
}

// center of keypad
void ScreenBuffer::escape_kb2(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
}

// backspace key
void ScreenBuffer::escape_kbs(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
}

// lower left of keypad
void ScreenBuffer::escape_kc1(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
}

// lower right of keypad
void ScreenBuffer::escape_kc3(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
}

// left-arrow key
void ScreenBuffer::escape_kcub1(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	*/
}

// down-arrow key
void ScreenBuffer::escape_kcud1(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	*/
}

// right-arrow key
void ScreenBuffer::escape_kcuf1(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	*/
}

// up-arrow key
void ScreenBuffer::escape_kcuu1(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	*/
}

// enter/send key
void ScreenBuffer::escape_kent(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	*/
}

// F0 function key
void ScreenBuffer::escape_kf0(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	*/
}

// F1 function key
void ScreenBuffer::escape_kf1(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	*/
}

// F10 function key
void ScreenBuffer::escape_kf10(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	*/
}

// F2 function key
void ScreenBuffer::escape_kf2(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	*/
}

// F3 function key
void ScreenBuffer::escape_kf3(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	*/
}

// F4 function key
void ScreenBuffer::escape_kf4(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	*/
}

// F5 function key
void ScreenBuffer::escape_kf5(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	*/
}

// F6 function key
void ScreenBuffer::escape_kf6(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	*/
}

// F7 function key
void ScreenBuffer::escape_kf7(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	*/
}

// F8 function key
void ScreenBuffer::escape_kf8(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	*/
}

// F9 function key
void ScreenBuffer::escape_kf9(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	*/
}

// label on function key f1 if not f1
void ScreenBuffer::escape_lf1(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	*/
}

// label on function key f2 if not f2
void ScreenBuffer::escape_lf2(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	*/
}

// label on function key f3 if not f3
void ScreenBuffer::escape_lf3(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	*/
}

// label on function key f4 if not f4
void ScreenBuffer::escape_lf4(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	*/
}

// print contents of screen
void ScreenBuffer::escape_mc0(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	*/
}

// turn off printer
void ScreenBuffer::escape_mc4(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	*/
}

// turn on printer
void ScreenBuffer::escape_mc5(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	*/
}

void ScreenBuffer::escape_nel(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	term->ccol=0;
	term->crow++;
	term->dirty_cursor.exists = true;
	if( term->crow >= term->grid.height ) {
		term_shiftrows_up(term);
		term_add_dirty_rect( term, 0, 0, term->grid.width, term->grid.height );
	}
	*/
}

// Restore cursor to position of last sc
void ScreenBuffer::escape_rc(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	term->crow = term->csavedrow;
	term->ccol = term->csavedcol;
	term->dirty_cursor.exists = true;
	*/
}
/*
// turn on reverse video mode
void ScreenBuffer::escape_rev(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG() << "turn on reverse video mode";
	m_currentAttributes |= ScreenCell::AttrReverse;
}
*/
// scroll text down
void ScreenBuffer::escape_ri(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	term_shiftrows_down(term);
	term_add_dirty_rect( term, 0, 0, term->grid.width, term->grid.height );
	*/
}

// end alternate character set
void ScreenBuffer::escape_rmacs(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG() << "end alternate character set NIY";
}

// turn off automatic margins
void ScreenBuffer::escape_rmam(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	*/
}

// Out of "keypad-transmit" mode
void ScreenBuffer::escape_rmkx(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	// Do nothing here, assume always transmit since we don't have a numeric keypad on VKB
}

// reset string
void ScreenBuffer::escape_rs2(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
}

// Save cursor position
void ScreenBuffer::escape_sc(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	term->csavedrow = term->crow;
	term->csavedcol = term->ccol;
	*/
}

// define video attributes #1-#9
void ScreenBuffer::escape_sgr(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	*/
}

// start alternate character set
void ScreenBuffer::escape_smacs(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
}

// turn on automatic margins
void ScreenBuffer::escape_smam(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
}

// Begin keypad transmit mode.
void ScreenBuffer::escape_smkx(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	// Do nothing here, assume always transmit since we don't have a numeric keypad on VKB
}

// clear all tab stops
void ScreenBuffer::escape_tbc(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	*/
}

// vertical position #1 absolute
void ScreenBuffer::escape_vpa(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	int vpos = atoi( term->output_bytes + 2 );
	if( vpos >= term->grid.height ) {
		term->crow = term->grid.height - 1;
	}  else {
		term->crow = vpos;
	}
	term->dirty_cursor.exists = true;
	*/
}

void ScreenBuffer::escape_sgm(const QStringList &params)
{
	Q_UNUSED(params);
	ESC_DEBUG_NIY();
	/*
	int i = 2;
	int val;
	char *nptr;

	while( true ) {
		val = strtoul( term->output_bytes + i, &nptr, 10 );
		switch( val ) {
			case 0:
				term->cattr = 0;
				break;
			case 1:
				term->cattr |= TERM_ATTRIB_BOLD;
				break;
			case 4:
				term->cattr |= TERM_ATTRIB_UNDERSCORE;
				break;
			case 5:
				term->cattr |= TERM_ATTRIB_BLINK;
				break;
			case 7:
				term->cattr |= TERM_ATTRIB_REVERSE;
				break;
			case 8:
				term->cattr |= TERM_ATTRIB_CONCEALED;
				break;
			case 30:
				// Black
				term->ccolour &= ~TERM_FG_MASK;
				term->ccolour |= TERM_COLOR_BLACK << TERM_FG_SHIFT;
				break;
			case 31:
				// Red
				term->ccolour &= ~TERM_FG_MASK;
				term->ccolour |= TERM_COLOR_RED << TERM_FG_SHIFT;
				break;
			case 32:
				// Green
				term->ccolour &= ~TERM_FG_MASK;
				term->ccolour |= TERM_COLOR_GREEN << TERM_FG_SHIFT;
				break;
			case 33:
				// Yellow
				term->ccolour &= ~TERM_FG_MASK;
				term->ccolour |= TERM_COLOR_YELLOW << TERM_FG_SHIFT;
				break;
			case 34:
				// Blue
				term->ccolour &= ~TERM_FG_MASK;
				term->ccolour |= TERM_COLOR_BLUE << TERM_FG_SHIFT;
				break;
			case 35:
				// Magenta
				term->ccolour &= ~TERM_FG_MASK;
				term->ccolour |= TERM_COLOR_MAGENTA << TERM_FG_SHIFT;
				break;
			case 36:
				// Cyan
				term->ccolour &= ~TERM_FG_MASK;
				term->ccolour |= TERM_COLOR_CYAN << TERM_FG_SHIFT;
				break;
			case 37:
				// White
				term->ccolour &= ~TERM_FG_MASK;
				term->ccolour |= TERM_COLOR_WHITE << TERM_FG_SHIFT;
				break;
			case 40:
				// Black
				term->ccolour &= ~TERM_BG_MASK;
				term->ccolour |= TERM_COLOR_BLACK << TERM_BG_SHIFT;
				break;
			case 41:
				// Red
				term->ccolour &= ~TERM_BG_MASK;
				term->ccolour |= TERM_COLOR_RED << TERM_BG_SHIFT;
				break;
			case 42:
				// Green
				term->ccolour &= ~TERM_BG_MASK;
				term->ccolour |= TERM_COLOR_GREEN << TERM_BG_SHIFT;
				break;
			case 43:
				// Yellow
				term->ccolour &= ~TERM_BG_MASK;
				term->ccolour |= TERM_COLOR_YELLOW << TERM_BG_SHIFT;
				break;
			case 44:
				// Blue
				term->ccolour &= ~TERM_BG_MASK;
				term->ccolour |= TERM_COLOR_BLUE << TERM_BG_SHIFT;
				break;
			case 45:
				// Magenta
				term->ccolour &= ~TERM_BG_MASK;
				term->ccolour |= TERM_COLOR_MAGENTA << TERM_BG_SHIFT;
				break;
			case 46:
				// Cyan
				term->ccolour &= ~TERM_BG_MASK;
				term->ccolour |= TERM_COLOR_CYAN << TERM_BG_SHIFT;
				break;
			case 47:
				// White
				term->ccolour &= ~TERM_BG_MASK;
				term->ccolour |= TERM_COLOR_WHITE << TERM_BG_SHIFT;
				break;
		}
		if( (*nptr) == 'm' ) break;
		i = nptr - term->output_bytes + 1;
	}
	*/
}

void ScreenBuffer::escape_charAttr(const QStringList &params)
{
	int n = params.value(0).toInt();
	ESC_DEBUG() << n;
	switch(n) {
	case 0:
		m_currentAttributes = 0;
		break;
	case 1:
		m_currentAttributes |= ScreenCell::AttrBright;
		break;
	case 2:
		m_currentAttributes |= ScreenCell::AttrDim;
		break;
	case 4:
		m_currentAttributes |= ScreenCell::AttrUnderscore;
		break;
	case 5:
		m_currentAttributes |= ScreenCell::AttrBlink;
		break;
	case 7:
		m_currentAttributes |= ScreenCell::AttrReverse;
		break;
	case 8:
		m_currentAttributes |= ScreenCell::AttrHidden;
		break;
	default:
		LOGWARN() << "invalid character attribute value:" << n;
	}
 }

static QMap<QString, QRegExp> seqRegExps;

int ScreenBuffer::processControlSequence(int start_pos)
{
	EscCommand *cmds = esc_commands_vt100;
	int cmdn = esc_commands_count_vt100;
	QStringList params;
	int cmd_index = -1;
	int matched_length = -1;
	for(int i=0; i<cmdn; i++) {
		EscCommand cmd = cmds[i];
		QLatin1String pattern = cmd.pattern;
		if(cmd.isRegexp) {
			QRegExp rx = seqRegExps.value(pattern);
			if(rx.isEmpty()) {
				rx = QRegExp(pattern);
				seqRegExps[pattern] = rx;
			}
			if(rx.indexIn(m_inputBuffer, start_pos, QRegExp::CaretAtOffset) == start_pos) {
				cmd_index = i;
				params = rx.capturedTexts().mid(1);
				matched_length = rx.matchedLength();
				break;
			}
		}
		else {
			QStringRef str_ref(&m_inputBuffer, start_pos, m_inputBuffer.length() - start_pos);
			if(str_ref.startsWith(pattern)) {
				cmd_index = i;
				matched_length = ::strlen(pattern.latin1());
				break;
			}
		}
	}
	if(cmd_index >= 0) {
		(*this.*(cmds[cmd_index].handler))(params);
		return matched_length;
	}
	return 0;
}
