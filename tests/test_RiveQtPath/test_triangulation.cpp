#include <QObject>
#include <QTest>

#include "riveqtpath.h"

class Test_PathTriangulation : public QObject
{
    Q_OBJECT

private slots:
    void test_doTrianglesOverlap()
    {
        QVector2D p1(1, 1), p2(10, 1), p3(5, 10);

        QVector2D t11(3, 3), t12(5, 3), t13(4, 4); // case 1 : completely inside
        QVector2D t21(5, 0), t22(10, 9), t23(1, 9); // case 2 : star configuration, no points covered
        QVector2D t31(3, 3), t32(13, 3), t33(8, 10); // case 3 : shifted
        QVector2D t41(5, 5), t42(6, 4), t43(10, 10); // case 4 : one edge inside
        QVector2D t51(3, 3), t52(7, 3), t53(5, 12); // case 5 : inside triangle covers one point
        QVector2D t61(0, 2), t62(0, 5), t63(10, 5); // case 6 : only the area covers, no points covered
        QVector2D t71(3, 0), t72(7, 0), t73(13, 5); // case 7 : one corner covered, all edges cut

        QVERIFY(RiveQtPath::doTrianglesOverlap(p1, p2, p3, t11, t12, t13) == true);
        QVERIFY(RiveQtPath::doTrianglesOverlap(p1, p2, p3, t21, t22, t23) == true);
        QVERIFY(RiveQtPath::doTrianglesOverlap(p1, p2, p3, t31, t32, t33) == true);
        QVERIFY(RiveQtPath::doTrianglesOverlap(p1, p2, p3, t41, t42, t43) == true);
        QVERIFY(RiveQtPath::doTrianglesOverlap(p1, p2, p3, t51, t52, t53) == true);
        QVERIFY(RiveQtPath::doTrianglesOverlap(p1, p2, p3, t61, t62, t63) == true);
        QVERIFY(RiveQtPath::doTrianglesOverlap(p1, p2, p3, t71, t72, t73) == true);

        QVector2D t81(15, 15), t82(20, 25), t83(25, 15); // case 8 : not covered
        QVERIFY(RiveQtPath::doTrianglesOverlap(p1, p2, p3, t81, t82, t83) == false);
    }

    void test_findOverlappingTriangles()
    {
        QVector<QVector2D> trianglePoints = { { 1, 1 },   { 10, 1 },  { 5, 10 },  { 5, 5 },  { 15, 5 },
                                              { 10, 15 }, { 15, 15 }, { 20, 25 }, { 25, 15 } };
        QVector<std::tuple<size_t, size_t>> expected { { 0, 1 } };
        const auto &result = RiveQtPath::findOverlappingTriangles(trianglePoints);
        QCOMPARE(RiveQtPath::findOverlappingTriangles(trianglePoints).size(), expected.size());
        for (size_t i = 0; i < result.size(); ++i) {
            QCOMPARE(result.at(i), expected.at(i));
        }
    }

    void test_splitTriangles()
    {
        QVector<QVector2D> trianglePoints = { { 1, 1 }, { 10, 1 }, { 5, 10 }, { 5, 5 }, { 15, 5 }, { 10, 15 } };
        const auto &result = RiveQtPath::splitTriangles(trianglePoints);
        if (result.size() == 3) {
            return;
        }
        QVERIFY(result.size() % 3 == 0);
        QVERIFY(RiveQtPath::doTrianglesOverlap(result.at(0), result.at(1), result.at(2), result.at(3), result.at(4), result.at(5))
                == false);
    }
};

QTEST_MAIN(Test_PathTriangulation)

#include "test_triangulation.moc"
