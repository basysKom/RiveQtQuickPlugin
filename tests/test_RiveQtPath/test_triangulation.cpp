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
        QVector2D p4(5, 5), p5(15, 5), p6(10, 15);
        QVERIFY(RiveQtPath::doTrianglesOverlap(p1, p2, p3, p4, p5, p6) == true);
        QVector2D p7(15, 15), p8(20, 25), p9(25, 15);
        QVERIFY(RiveQtPath::doTrianglesOverlap(p1, p2, p3, p7, p8, p9) == false);
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
