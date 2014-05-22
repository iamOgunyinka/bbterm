#ifndef BBVIRTUALKEYBOARDHANDLER_H
#define BBVIRTUALKEYBOARDHANDLER_H

#include <QObject>
#include <QAbstractEventDispatcher>

namespace gui {
namespace qt {

class BBVirtualKeyboardHandler : public QObject
{
	Q_OBJECT
	Q_PROPERTY(bool keyboardVisible READ isKeyboardVisible WRITE setKeyboardVisible NOTIFY keyboardVisibleChanged)
private:
	explicit BBVirtualKeyboardHandler(QObject *parent = 0);
public:
	static BBVirtualKeyboardHandler* instance();

	bool isKeyboardVisible() const {return m_keyboardVisible;}
	void setKeyboardVisible(bool b) {m_keyboardVisible = b; emit keyboardVisibleChanged(m_keyboardVisible);}
	Q_SIGNAL void keyboardVisibleChanged(bool visible);

	int keyboardHeight();
private:
	bool m_keyboardVisible;
	static bool eventFilter(void *message);
	static QAbstractEventDispatcher::EventFilter m_prevEventFilter;
};

}
}

#endif // BBVIRTUALKEYBOARDHANDLER_H