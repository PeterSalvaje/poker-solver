#ifndef CUSTOMSLIDER_H
#define CUSTOMSLIDER_H

#include <QSlider>

class MySlider : public QSlider {
public:
    using QSlider::QSlider;

protected:
    void mousePressEvent(QMouseEvent* event) override;
};

#endif // CUSTOMSLIDER_H
