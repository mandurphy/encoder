/********************************************************************************
** Form generated from reading UI file 'dash.ui'
**
** Created by: Qt User Interface Compiler version 5.5.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DASH_H
#define UI_DASH_H

#include <QtCore/QVariant>
#include <QtWidgets/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Dash
{
public:
    QWidget *RuleDash;
    QHBoxLayout *horizontalLayout_3;
    QVBoxLayout *rule_v_Layout1;
    QHBoxLayout *rule_h_layout1;
    QSpacerItem *rule_h_spacer1_1;
    QLabel *rule_label_ip;
    QSpacerItem *rule_h_spacer1_2;
    QSpacerItem *rule_v_spacer1;
    QLabel *rule_label_hdmi;
    QSpacerItem *rule_v_spacer2;
    QHBoxLayout *rule_h_layout2;
    QLabel *rule_label_audio;
    QLabel *rule_label_bitrate;
    QSpacerItem *rule_v_spacer3;
    QHBoxLayout *rule_h_layout3;
    QLabel *rule_label_cpu;
    QLabel *rule_label_temp;

    void setupUi(QWidget *Dash)
    {
        if (Dash->objectName().isEmpty())
            Dash->setObjectName(QStringLiteral("Dash"));
        Dash->setEnabled(true);
        Dash->resize(1051, 758);
        QFont font;
        font.setPointSize(6);
        Dash->setFont(font);
        Dash->setStyleSheet(QStringLiteral(""));
        RuleDash = new QWidget(Dash);
        RuleDash->setObjectName(QStringLiteral("RuleDash"));
        RuleDash->setGeometry(QRect(60, 50, 256, 128));
        RuleDash->setMaximumSize(QSize(256, 128));
        horizontalLayout_3 = new QHBoxLayout(RuleDash);
        horizontalLayout_3->setObjectName(QStringLiteral("horizontalLayout_3"));
        horizontalLayout_3->setContentsMargins(0, 0, 0, 0);
        rule_v_Layout1 = new QVBoxLayout();
        rule_v_Layout1->setSpacing(0);
        rule_v_Layout1->setObjectName(QStringLiteral("rule_v_Layout1"));
        rule_v_Layout1->setSizeConstraint(QLayout::SetFixedSize);
        rule_h_layout1 = new QHBoxLayout();
        rule_h_layout1->setSpacing(0);
        rule_h_layout1->setObjectName(QStringLiteral("rule_h_layout1"));
        rule_h_spacer1_1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        rule_h_layout1->addItem(rule_h_spacer1_1);

        rule_label_ip = new QLabel(RuleDash);
        rule_label_ip->setObjectName(QStringLiteral("rule_label_ip"));
        QFont font1;
        font1.setPointSize(11);
        rule_label_ip->setFont(font1);
        rule_label_ip->setStyleSheet(QStringLiteral(""));

        rule_h_layout1->addWidget(rule_label_ip, 0, Qt::AlignHCenter);

        rule_h_spacer1_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        rule_h_layout1->addItem(rule_h_spacer1_2);

        rule_h_layout1->setStretch(0, 6);
        rule_h_layout1->setStretch(1, 2);
        rule_h_layout1->setStretch(2, 6);

        rule_v_Layout1->addLayout(rule_h_layout1);

        rule_v_spacer1 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        rule_v_Layout1->addItem(rule_v_spacer1);

        rule_label_hdmi = new QLabel(RuleDash);
        rule_label_hdmi->setObjectName(QStringLiteral("rule_label_hdmi"));
        QFont font2;
        font2.setPointSize(10);
        rule_label_hdmi->setFont(font2);
        rule_label_hdmi->setStyleSheet(QStringLiteral(""));

        rule_v_Layout1->addWidget(rule_label_hdmi);

        rule_v_spacer2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        rule_v_Layout1->addItem(rule_v_spacer2);

        rule_h_layout2 = new QHBoxLayout();
        rule_h_layout2->setObjectName(QStringLiteral("rule_h_layout2"));
        rule_label_audio = new QLabel(RuleDash);
        rule_label_audio->setObjectName(QStringLiteral("rule_label_audio"));
        rule_label_audio->setFont(font2);

        rule_h_layout2->addWidget(rule_label_audio);

        rule_label_bitrate = new QLabel(RuleDash);
        rule_label_bitrate->setObjectName(QStringLiteral("rule_label_bitrate"));
        rule_label_bitrate->setFont(font2);

        rule_h_layout2->addWidget(rule_label_bitrate);

        rule_h_layout2->setStretch(0, 1);
        rule_h_layout2->setStretch(1, 1);

        rule_v_Layout1->addLayout(rule_h_layout2);

        rule_v_spacer3 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        rule_v_Layout1->addItem(rule_v_spacer3);

        rule_h_layout3 = new QHBoxLayout();
        rule_h_layout3->setSpacing(7);
        rule_h_layout3->setObjectName(QStringLiteral("rule_h_layout3"));
        rule_label_cpu = new QLabel(RuleDash);
        rule_label_cpu->setObjectName(QStringLiteral("rule_label_cpu"));
        rule_label_cpu->setFont(font2);

        rule_h_layout3->addWidget(rule_label_cpu);

        rule_label_temp = new QLabel(RuleDash);
        rule_label_temp->setObjectName(QStringLiteral("rule_label_temp"));
        rule_label_temp->setFont(font2);

        rule_h_layout3->addWidget(rule_label_temp);

        rule_h_layout3->setStretch(0, 1);
        rule_h_layout3->setStretch(1, 1);

        rule_v_Layout1->addLayout(rule_h_layout3);


        horizontalLayout_3->addLayout(rule_v_Layout1);


        retranslateUi(Dash);

        QMetaObject::connectSlotsByName(Dash);
    } // setupUi

    void retranslateUi(QWidget *Dash)
    {
        Dash->setWindowTitle(QApplication::translate("Dash", "Form", 0));
        rule_label_ip->setText(QApplication::translate("Dash", "127.0.0.1", 0));
        rule_label_hdmi->setText(QApplication::translate("Dash", "HDMI: No input", 0));
        rule_label_audio->setText(QApplication::translate("Dash", "Aud: 0db", 0));
        rule_label_bitrate->setText(QApplication::translate("Dash", "BR:0k", 0));
        rule_label_cpu->setText(QApplication::translate("Dash", "CPU: 20%", 0));
        rule_label_temp->setText(QApplication::translate("Dash", "TEMP:39", 0));
    } // retranslateUi

};

namespace Ui {
    class Dash: public Ui_Dash {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DASH_H
