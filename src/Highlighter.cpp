#include "Highlighter.h"

Highlighter::Highlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    HighlightingRule rule;

    keywordFormat.setForeground(Qt::green);
    keywordFormat.setFontWeight(QFont::Bold);
    QStringList keywordPatterns;

//    << "\\bDRAW_LINES\\b"
//                        << "\\bDRAW_POLYGON\\b"
//                        << "\\bSVG_LOAD\\b"
//                        << "\\bFRACTALE_TREE\\b"
//                        << "\\bFRACTALE_VON_KOCH\\b"
//                        << "\\bSET_FONT\\b"
//                        << "\\bDRAW_TEXT\\b"
//                        << "\\bTRANSFORM_ROTATE\\b"
//                        << "\\bTRANSFORM_SCALE\\b"
//                        << "\\bDRAW_LINE_ROSACE\\b"
//                        << "\\bDRAW_LINE_ROSACE_W\\b"
//                        << "\\bIMG_LOAD\\b"
//                        << "\\bTRANSFORM_TRANSLATE\\b";

    for(int i=0;i<NB_CMD;i++)
    keywordPatterns.append(CommandsList[i].keyword);

    foreach (const QString &pattern, keywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        highlightingRules.append(rule);
    }

    subkeywordFormat.setForeground(Qt::cyan);
    subkeywordFormat.setFontWeight(QFont::Bold);
    QStringList subkeywordPatterns;
    subkeywordPatterns << "\\bNone\\b"<< "\\bSolid\\b"<< "\\bDash\\b" << "\\bDot\\b"<<"\\bMath\\b"<<"\\bArial\\b"<<"\\bHelvetica\\b"<<"\\bTimes\\b";

    foreach (const QString &pattern, subkeywordPatterns) {
        rule.pattern = QRegularExpression(pattern);
        rule.format = subkeywordFormat;
        highlightingRules.append(rule);
    }

    singleLineCommentFormat.setForeground(Qt::darkGreen);
    rule.pattern = QRegularExpression("//[^\n]*");
    rule.format = singleLineCommentFormat;
    highlightingRules.append(rule);

    multiLineCommentFormat.setForeground(Qt::darkGreen);

    chiffresFormat.setForeground(Qt::white);
    rule.pattern = QRegularExpression("[0-9]");
    rule.format = chiffresFormat;
    highlightingRules.append(rule);

    commentStartExpression = QRegularExpression("/\\*");
    commentEndExpression = QRegularExpression("\\*/");
}

void Highlighter::highlightBlock(const QString &text)
{
    foreach (const HighlightingRule &rule, highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
    setCurrentBlockState(0);

    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = text.indexOf(commentStartExpression);

    while (startIndex >= 0) {
        QRegularExpressionMatch match = commentEndExpression.match(text, startIndex);
        int endIndex = match.capturedStart();
        int commentLength = 0;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex
                    + match.capturedLength();
        }
        setFormat(startIndex, commentLength, multiLineCommentFormat);
        startIndex = text.indexOf(commentStartExpression, startIndex + commentLength);
    }
}
