// SPDX-FileCopyrightText: 2016 John Burkardt https://people.sc.fsu.edu/~jburkardt/cpp_src/polygon_triangulate/polygon_triangulate.html
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>

#include "polygon_triangulate/polygon_triangulate.hpp"

using namespace std;

//****************************************************************************80

double angle_degree(double x1, double y1, double x2, double y2, double x3, double y3)

//****************************************************************************80
//
//  Purpose:
//
//    ANGLE_DEGREE returns the degree angle defined by three points.
//
//  Discussion:
//
//        P1
//        /
//       /
//      /
//     /
//    P2--------->P3
//
//  Licensing:
//
//    This code is distributed under the GNU LGPL license.
//
//  Modified:
//
//    28 August 2016
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, double X1, Y1, X2, Y2, X3, Y3, the coordinates of the points
//    P1, P2, P3.
//
//    Output, double ANGLE_DEGREE, the angle swept out by the rays, measured
//    in degrees.  0 <= VALUE < 360.  If either ray has zero length,
//    then VALUE is set to 0.
//
{
    double r8_pi = 3.141592653589793;
    double value;
    double x;
    double y;

    x = (x3 - x2) * (x1 - x2) + (y3 - y2) * (y1 - y2);

    y = (x3 - x2) * (y1 - y2) - (y3 - y2) * (x1 - x2);

    if (x == 0.0 && y == 0.0) {
        value = 0.0;
    } else {
        value = atan2(y, x);

        if (value < 0.0) {
            value = value + 2.0 * r8_pi;
        }
        value = 180.0 * value / r8_pi;
    }

    return value;
}
//****************************************************************************80

bool between(double xa, double ya, double xb, double yb, double xc, double yc)

//****************************************************************************80
//
//  Purpose:
//
//    BETWEEN is TRUE if vertex C is between vertices A and B.
//
//  Discussion:
//
//    The points must be (numerically) collinear.
//
//    Given that condition, we take the greater of XA - XB and YA - YB
//    as a "scale" and check where C's value lies.
//
//  Licensing:
//
//    This code is distributed under the GNU LGPL license.
//
//  Modified:
//
//    05 May 2014
//
//  Author:
//
//    Original C version by Joseph ORourke.
//    This C++ version by John Burkardt.
//
//  Reference:
//
//    Joseph ORourke,
//    Computational Geometry in C,
//    Cambridge, 1998,
//    ISBN: 0521649765,
//    LC: QA448.D38.
//
//  Parameters:
//
//    Input, double XA, YA, XB, YB, XC, YC, the coordinates of
//    the vertices.
//
//    Output, bool BETWEEN, is TRUE if C is between A and B.
//
{
    bool value;
    double xmax;
    double xmin;
    double ymax;
    double ymin;

    if (!collinear(xa, ya, xb, yb, xc, yc)) {
        value = false;
    } else if (fabs(ya - yb) < fabs(xa - xb)) {
        xmax = fmax(xa, xb);
        xmin = fmin(xa, xb);
        value = (xmin <= xc && xc <= xmax);
    } else {
        ymax = fmax(ya, yb);
        ymin = fmin(ya, yb);
        value = (ymin <= yc && yc <= ymax);
    }

    return value;
}
//****************************************************************************80

char ch_cap(char ch)

//****************************************************************************80
//
//  Purpose:
//
//    CH_CAP capitalizes a single character.
//
//  Discussion:
//
//    This routine should be equivalent to the library "toupper" function.
//
//  Licensing:
//
//    This code is distributed under the GNU LGPL license.
//
//  Modified:
//
//    19 July 1998
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, char CH, the character to capitalize.
//
//    Output, char CH_CAP, the capitalized character.
//
{
    if (97 <= ch && ch <= 122) {
        ch = ch - 32;
    }

    return ch;
}
//****************************************************************************80

bool ch_eqi(char ch1, char ch2)

//****************************************************************************80
//
//  Purpose:
//
//    CH_EQI is true if two characters are equal, disregarding case.
//
//  Licensing:
//
//    This code is distributed under the GNU LGPL license.
//
//  Modified:
//
//    13 June 2003
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, char CH1, CH2, the characters to compare.
//
//    Output, bool CH_EQI, is true if the two characters are equal,
//    disregarding case.
//
{
    if (97 <= ch1 && ch1 <= 122) {
        ch1 = ch1 - 32;
    }
    if (97 <= ch2 && ch2 <= 122) {
        ch2 = ch2 - 32;
    }

    return (ch1 == ch2);
}
//****************************************************************************80

int ch_to_digit(char ch)

//****************************************************************************80
//
//  Purpose:
//
//    CH_TO_DIGIT returns the integer value of a base 10 digit.
//
//  Example:
//
//     CH  DIGIT
//    ---  -----
//    '0'    0
//    '1'    1
//    ...  ...
//    '9'    9
//    ' '    0
//    'X'   -1
//
//  Licensing:
//
//    This code is distributed under the GNU LGPL license.
//
//  Modified:
//
//    13 June 2003
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, char CH, the decimal digit, '0' through '9' or blank are legal.
//
//    Output, int CH_TO_DIGIT, the corresponding integer value.  If the
//    character was 'illegal', then DIGIT is -1.
//
{
    int digit;

    if ('0' <= ch && ch <= '9') {
        digit = ch - '0';
    } else if (ch == ' ') {
        digit = 0;
    } else {
        digit = -1;
    }

    return digit;
}
//****************************************************************************80

bool collinear(double xa, double ya, double xb, double yb, double xc, double yc)

//****************************************************************************80
//
//  Purpose:
//
//    COLLINEAR returns a measure of collinearity for three points.
//
//  Discussion:
//
//    In order to deal with collinear points whose coordinates are not
//    numerically exact, we compare the area of the largest square
//    that can be created by the line segment between two of the points
//    to (twice) the area of the triangle formed by the points.
//
//    If the points are collinear, their triangle has zero area.
//    If the points are close to collinear, then the area of this triangle
//    will be small relative to the square of the longest segment.
//
//  Licensing:
//
//    This code is distributed under the GNU LGPL license.
//
//  Modified:
//
//    10 September 2016
//
//  Author:
//
//    Original C version by Joseph ORourke.
//    C++ version by John Burkardt.
//
//  Reference:
//
//    Joseph ORourke,
//    Computational Geometry in C,
//    Cambridge, 1998,
//    ISBN: 0521649765,
//    LC: QA448.D38.
//
//  Parameters:
//
//    Input, double XA, YA, XB, YB, XC, YC, the vertex coordinates.
//
//    Output, bool COLLINEAR, is TRUE if the points are judged
//    to be collinear.
//
{
    double area;
    const double r8_eps = 2.220446049250313E-016;
    double side_ab_sq;
    double side_bc_sq;
    double side_ca_sq;
    double side_max_sq;
    bool value;

    area = triangle_area(xa, ya, xb, yb, xc, yc);

    side_ab_sq = pow(xa - xb, 2) + pow(ya - yb, 2);
    side_bc_sq = pow(xb - xc, 2) + pow(yb - yc, 2);
    side_ca_sq = pow(xc - xa, 2) + pow(yc - ya, 2);

    side_max_sq = fmax(side_ab_sq, fmax(side_bc_sq, side_ca_sq));

    if (side_max_sq <= r8_eps) {
        value = true;
    } else if (2.0 * fabs(area) <= r8_eps * side_max_sq) {
        value = true;
    } else {
        value = false;
    }

    return value;
}
//****************************************************************************80

bool diagonal(int im1, int ip1, int n, int prev_node[], int next_node[], double x[], double y[])

//****************************************************************************80
//
//  Purpose:
//
//    DIAGONAL: VERTEX(IM1) to VERTEX(IP1) is a proper internal diagonal.
//
//  Licensing:
//
//    This code is distributed under the GNU LGPL license.
//
//  Modified:
//
//    05 May 2014
//
//  Author:
//
//    Original C version by Joseph ORourke.
//    C++ version by John Burkardt.
//
//  Reference:
//
//    Joseph ORourke,
//    Computational Geometry in C,
//    Cambridge, 1998,
//    ISBN: 0521649765,
//    LC: QA448.D38.
//
//  Parameters:
//
//    Input, int IM1, IP1, the indices of two vertices.
//
//    Input, int N, the number of vertices.
//
//    Input, int PREV_NODE[N], the previous neighbor of each vertex.
//
//    Input, int NEXT_NODE[N], the next neighbor of each vertex.
//
//    Input, double X[N], Y[N], the coordinates of each vertex.
//
//    Output, bool DIAGONAL, the value of the test.
//
{
    bool value;
    bool value1;
    bool value2;
    bool value3;

    value1 = in_cone(im1, ip1, n, prev_node, next_node, x, y);
    value2 = in_cone(ip1, im1, n, prev_node, next_node, x, y);
    value3 = diagonalie(im1, ip1, n, next_node, x, y);

    value = (value1 && value2 && value3);

    return value;
}
//****************************************************************************80

bool diagonalie(int im1, int ip1, int n, int next_node[], double x[], double y[])

//****************************************************************************80
//
//  Purpose:
//
//    DIAGONALIE is true if VERTEX(IM1):VERTEX(IP1) is a proper diagonal.
//
//  Licensing:
//
//    This code is distributed under the GNU LGPL license.
//
//  Modified:
//
//    05 May 2014
//
//  Author:
//
//    Original C version by Joseph ORourke.
//    This C++ version by John Burkardt.
//
//  Reference:
//
//    Joseph ORourke,
//    Computational Geometry in C,
//    Cambridge, 1998,
//    ISBN: 0521649765,
//    LC: QA448.D38.
//
//  Parameters:
//
//    Input, int IM1, IP1, the indices of two vertices.
//
//    Input, int N, the number of vertices.
//
//    Input, int NEXT_NODE[N], the next neighbor of each vertex.
//
//    Input, double X[N], Y[N], the coordinates of each vertex.
//
//    Output, bool DIAGONALIE, the value of the test.
//
{
    int first;
    int j;
    int jp1;
    bool value;
    bool value2;

    first = im1;
    j = first;
    jp1 = next_node[first];

    value = true;
    //
    //  For each edge VERTEX(J):VERTEX(JP1) of the polygon:
    //
    while (1) {
        //
        //  Skip any edge that includes vertex IM1 or IP1.
        //
        if (j == im1 || j == ip1 || jp1 == im1 || jp1 == ip1) {
        } else {
            value2 = intersect(x[im1], y[im1], x[ip1], y[ip1], x[j], y[j], x[jp1], y[jp1]);

            if (value2) {
                value = false;
                break;
            }
        }
        j = jp1;
        jp1 = next_node[j];

        if (j == first) {
            break;
        }
    }

    return value;
}
//****************************************************************************80

int file_column_count(string filename)

//****************************************************************************80
//
//  Purpose:
//
//    FILE_COLUMN_COUNT counts the columns in the first line of a file.
//
//  Discussion:
//
//    The file is assumed to be a simple text file.
//
//    Most lines of the file are presumed to consist of COLUMN_NUM words,
//    separated by spaces.  There may also be some blank lines, and some
//    comment lines, which have a "#" in column 1.
//
//    The routine tries to find the first non-comment non-blank line and
//    counts the number of words in that line.
//
//    If all lines are blanks or comments, it goes back and tries to analyze
//    a comment line.
//
//  Licensing:
//
//    This code is distributed under the GNU LGPL license.
//
//  Modified:
//
//    05 July 2009
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, string FILENAME, the name of the file.
//
//    Output, int FILE_COLUMN_COUNT, the number of columns assumed
//    to be in the file.
//
{
    int column_num;
    ifstream input;
    bool got_one;
    string text;
    //
    //  Open the file.
    //
    input.open(filename.c_str());

    if (!input) {
        column_num = -1;
        cerr << "\n";
        cerr << "FILE_COLUMN_COUNT - Fatal error!\n";
        cerr << "  Could not open the file:\n";
        cerr << "  \"" << filename << "\"\n";
        exit(1);
    }
    //
    //  Read one line, but skip blank lines and comment lines.
    //
    got_one = false;

    for (;;) {
        getline(input, text);

        if (input.eof()) {
            break;
        }

        if (s_len_trim(text) <= 0) {
            continue;
        }

        if (text[0] == '#') {
            continue;
        }
        got_one = true;
        break;
    }

    if (!got_one) {
        input.close();

        input.open(filename.c_str());

        for (;;) {
            input >> text;

            if (input.eof()) {
                break;
            }

            if (s_len_trim(text) == 0) {
                continue;
            }
            got_one = true;
            break;
        }
    }

    input.close();

    if (!got_one) {
        cerr << "\n";
        cerr << "FILE_COLUMN_COUNT - Warning!\n";
        cerr << "  The file does not seem to contain any data.\n";
        return -1;
    }

    column_num = s_word_count(text);

    return column_num;
}
//****************************************************************************80

int file_row_count(string input_filename)

//****************************************************************************80
//
//  Purpose:
//
//    FILE_ROW_COUNT counts the number of row records in a file.
//
//  Discussion:
//
//    It does not count lines that are blank, or that begin with a
//    comment symbol '#'.
//
//  Licensing:
//
//    This code is distributed under the GNU LGPL license.
//
//  Modified:
//
//    23 February 2009
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, string INPUT_FILENAME, the name of the input file.
//
//    Output, int FILE_ROW_COUNT, the number of rows found.
//
{
    int comment_num;
    ifstream input;
    string line;
    int record_num;
    int row_num;

    row_num = 0;
    comment_num = 0;
    record_num = 0;

    input.open(input_filename.c_str());

    if (!input) {
        cerr << "\n";
        cerr << "FILE_ROW_COUNT - Fatal error!\n";
        cerr << "  Could not open the input file: \"" << input_filename << "\"\n";
        exit(1);
    }

    for (;;) {
        getline(input, line);

        if (input.eof()) {
            break;
        }

        record_num = record_num + 1;

        if (line[0] == '#') {
            comment_num = comment_num + 1;
            continue;
        }

        if (s_len_trim(line) == 0) {
            comment_num = comment_num + 1;
            continue;
        }

        row_num = row_num + 1;
    }

    input.close();

    return row_num;
}
//****************************************************************************80

void i4mat_transpose_print(int m, int n, int a[], string title)

//****************************************************************************80
//
//  Purpose:
//
//    I4MAT_TRANSPOSE_PRINT prints an I4MAT, transposed.
//
//  Discussion:
//
//    An I4MAT is an MxN array of I4's, stored by (I,J) -> [I+J*M].
//
//  Licensing:
//
//    This code is distributed under the GNU LGPL license.
//
//  Modified:
//
//    31 January 2005
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, int M, the number of rows in A.
//
//    Input, int N, the number of columns in A.
//
//    Input, int A[M*N], the M by N matrix.
//
//    Input, string TITLE, a title.
//
{
    i4mat_transpose_print_some(m, n, a, 1, 1, m, n, title);

    return;
}
//****************************************************************************80

void i4mat_transpose_print_some(int m, int n, int a[], int ilo, int jlo, int ihi, int jhi, string title)

//****************************************************************************80
//
//  Purpose:
//
//    I4MAT_TRANSPOSE_PRINT_SOME prints some of an I4MAT, transposed.
//
//  Discussion:
//
//    An I4MAT is an MxN array of I4's, stored by (I,J) -> [I+J*M].
//
//  Licensing:
//
//    This code is distributed under the GNU LGPL license.
//
//  Modified:
//
//    15 October 2014
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, int M, the number of rows of the matrix.
//    M must be positive.
//
//    Input, int N, the number of columns of the matrix.
//    N must be positive.
//
//    Input, int A[M*N], the matrix.
//
//    Input, int ILO, JLO, IHI, JHI, designate the first row and
//    column, and the last row and column to be printed.
//
//    Input, string TITLE, a title.
//
{
#define INCX 10

    int i;
    int i2hi;
    int i2lo;
    int j;
    int j2hi;
    int j2lo;

    cout << "\n";
    cout << title << "\n";

    if (m <= 0 || n <= 0) {
        cout << "\n";
        cout << "  (None)\n";
        return;
    }
    //
    //  Print the columns of the matrix, in strips of INCX.
    //
    for (i2lo = ilo; i2lo <= ihi; i2lo = i2lo + INCX) {
        i2hi = i2lo + INCX - 1;
        if (m < i2hi) {
            i2hi = m;
        }
        if (ihi < i2hi) {
            i2hi = ihi;
        }
        cout << "\n";
        //
        //  For each row I in the current range...
        //
        //  Write the header.
        //
        cout << "  Row: ";
        for (i = i2lo; i <= i2hi; i++) {
            cout << setw(6) << i - 1 << "  ";
        }
        cout << "\n";
        cout << "  Col\n";
        cout << "\n";
        //
        //  Determine the range of the rows in this strip.
        //
        j2lo = jlo;
        if (j2lo < 1) {
            j2lo = 1;
        }
        j2hi = jhi;
        if (n < j2hi) {
            j2hi = n;
        }
        for (j = j2lo; j <= j2hi; j++) {
            //
            //  Print out (up to INCX) entries in column J, that lie in the current strip.
            //
            cout << setw(5) << j - 1 << ":";
            for (i = i2lo; i <= i2hi; i++) {
                cout << setw(6) << a[i - 1 + (j - 1) * m] << "  ";
            }
            cout << "\n";
        }
    }

    return;
#undef INCX
}
//****************************************************************************80

void i4mat_write(string output_filename, int m, int n, int table[])

//****************************************************************************80
//
//  Purpose:
//
//    I4MAT_WRITE writes an I4MAT file with no header.
//
//  Discussion:
//
//    An I4MAT is an array of I4's.
//
//  Licensing:
//
//    This code is distributed under the GNU LGPL license.
//
//  Modified:
//
//    01 June 2009
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, string OUTPUT_FILENAME, the output filename.
//
//    Input, int M, the spatial dimension.
//
//    Input, int N, the number of points.
//
//    Input, int TABLE[M*N], the data.
//
{
    int i;
    int j;
    ofstream output;
    //
    //  Open the file.
    //
    output.open(output_filename.c_str());

    if (!output) {
        cerr << "\n";
        cerr << "I4MAT_WRITE - Fatal error!\n";
        cerr << "  Could not open the output file.\n";
        exit(1);
    }
    //
    //  Write the data.
    //
    for (j = 0; j < n; j++) {
        for (i = 0; i < m; i++) {
            output << "  " << setw(10) << table[i + j * m];
        }
        output << "\n";
    }
    //
    //  Close the file.
    //
    output.close();

    return;
}
//****************************************************************************80

bool in_cone(int im1, int ip1, int n, int prev_node[], int next_node[], double x[], double y[])

//****************************************************************************80
//
//  Purpose:
//
//    IN_CONE is TRUE if the diagonal VERTEX(IM1):VERTEX(IP1) is strictly internal.
//
//  Licensing:
//
//    This code is distributed under the GNU LGPL license.
//
//  Modified:
//
//    05 May 2014
//
//  Author:
//
//    Original C version by Joseph ORourke.
//    C++ version by John Burkardt.
//
//  Reference:
//
//    Joseph ORourke,
//    Computational Geometry in C,
//    Cambridge, 1998,
//    ISBN: 0521649765,
//    LC: QA448.D38.
//
//  Parameters:
//
//    Input, int IM1, IP1, the indices of two vertices.
//
//    Input, int N, the number of vertices.
//
//    Input, int PREV_NODE[N], the previous neighbor of each vertex.
//
//    Input, int NEXT_NODE[N], the next neighbor of each vertex.
//
//    Input, double X[N], Y[N], the coordinates of each vertex.
//
//    Output, bool IN_CONE, the value of the test.
//
{
    int i;
    int im2;
    double t1;
    double t2;
    double t3;
    double t4;
    double t5;
    bool value;

    im2 = prev_node[im1];
    i = next_node[im1];

    t1 = triangle_area(x[im1], y[im1], x[i], y[i], x[im2], y[im2]);

    if (0.0 <= t1) {
        t2 = triangle_area(x[im1], y[im1], x[ip1], y[ip1], x[im2], y[im2]);
        t3 = triangle_area(x[ip1], y[ip1], x[im1], y[im1], x[i], y[i]);
        value = ((0.0 < t2) && (0.0 < t3));
    } else {
        t4 = triangle_area(x[im1], y[im1], x[ip1], y[ip1], x[i], y[i]);
        t5 = triangle_area(x[ip1], y[ip1], x[im1], y[im1], x[im2], y[im2]);
        value = !((0.0 <= t4) && (0.0 <= t5));
    }
    return value;
}
//****************************************************************************80

bool intersect(double xa, double ya, double xb, double yb, double xc, double yc, double xd, double yd)

//****************************************************************************80
//
//  Purpose:
//
//    INTERSECT is true if lines VA:VB and VC:VD intersect.
//
//  Licensing:
//
//    This code is distributed under the GNU LGPL license.
//
//  Modified:
//
//    05 May 2014
//
//  Author:
//
//    Original C version by Joseph ORourke.
//    C++ version by John Burkardt.
//
//  Reference:
//
//    Joseph ORourke,
//    Computational Geometry in C,
//    Cambridge, 1998,
//    ISBN: 0521649765,
//    LC: QA448.D38.
//
//  Parameters:
//
//    Input, double XA, YA, XB, YB, XC, YC, XD, YD, the X and Y
//    coordinates of the four vertices.
//
//    Output, bool INTERSECT, the value of the test.
//
{
    bool value;

    if (intersect_prop(xa, ya, xb, yb, xc, yc, xd, yd)) {
        value = true;
    } else if (between(xa, ya, xb, yb, xc, yc)) {
        value = true;
    } else if (between(xa, ya, xb, yb, xd, yd)) {
        value = true;
    } else if (between(xc, yc, xd, yd, xa, ya)) {
        value = true;
    } else if (between(xc, yc, xd, yd, xb, yb)) {
        value = true;
    } else {
        value = false;
    }
    return value;
}
//****************************************************************************80

bool intersect_prop(double xa, double ya, double xb, double yb, double xc, double yc, double xd, double yd)

//****************************************************************************80
//
//  Purpose:
//
//    INTERSECT_PROP is TRUE if lines VA:VB and VC:VD have a proper intersection.
//
//  Discussion:
//
//    Thanks to Gene Dial for correcting the call to intersect_prop(),
//    08 September 2016.
//
//  Licensing:
//
//    This code is distributed under the GNU LGPL license.
//
//  Modified:
//
//    08 September 2016
//
//  Author:
//
//    Original C version by Joseph ORourke.
//    This C++ version by John Burkardt.
//
//  Reference:
//
//    Joseph ORourke,
//    Computational Geometry in C,
//    Cambridge, 1998,
//    ISBN: 0521649765,
//    LC: QA448.D38.
//
//  Parameters:
//
//    Input, double XA, YA, XB, YB, XC, YC, XD, YD, the X and Y
//    coordinates of the four vertices.
//
//    Output, bool INTERSECT_PROP, the result of the test.
//
{
    double t1;
    double t2;
    double t3;
    double t4;
    bool value;
    bool value1;
    bool value2;
    bool value3;
    bool value4;

    if (collinear(xa, ya, xb, yb, xc, yc)) {
        value = false;
    } else if (collinear(xa, ya, xb, yb, xd, yd)) {
        value = false;
    } else if (collinear(xc, yc, xd, yd, xa, ya)) {
        value = false;
    } else if (collinear(xc, yc, xd, yd, xb, yb)) {
        value = false;
    } else {
        t1 = triangle_area(xa, ya, xb, yb, xc, yc);
        t2 = triangle_area(xa, ya, xb, yb, xd, yd);
        t3 = triangle_area(xc, yc, xd, yd, xa, ya);
        t4 = triangle_area(xc, yc, xd, yd, xb, yb);

        value1 = (0.0 < t1);
        value2 = (0.0 < t2);
        value3 = (0.0 < t3);
        value4 = (0.0 < t4);

        value = (l4_xor(value1, value2)) && (l4_xor(value3, value4));
    }
    return value;
}
//****************************************************************************80

bool l4_xor(bool l1, bool l2)

//****************************************************************************80
//
//  Purpose:
//
//    L4_XOR returns the exclusive OR of two L4's.
//
//  Discussion:
//
//    An L4 is a logical value.
//
//  Licensing:
//
//    This code is distributed under the GNU LGPL license.
//
//  Modified:
//
//    04 May 2014
//
//  Author:
//
//   John Burkardt
//
//  Parameters:
//
//    Input, bool L1, L2, two values whose exclusive OR is needed.
//
//    Output, bool L4_XOR, the exclusive OR of L1 and L2.
//
{
    bool value;
    bool value1;
    bool value2;

    value1 = (l1 && (!l2));
    value2 = ((!l1) && l2);

    value = (value1 || value2);

    return value;
}
//****************************************************************************80

void l4vec_print(int n, bool a[], string title)

//****************************************************************************80
//
//  Purpose:
//
//    L4VEC_PRINT prints an L4VEC.
//
//  Licensing:
//
//    This code is distributed under the GNU LGPL license.
//
//  Modified:
//
//    03 April 2005
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, int N, the number of components of the vector.
//
//    Input, bool A[N], the vector to be printed.
//
//    Input, string TITLE, a title.
//
{
    int i;

    cout << "\n";
    cout << title << "\n";
    cout << "\n";
    for (i = 0; i < n; i++) {
        cout << "  " << setw(8) << i << ": " << setw(1) << a[i] << "\n";
    }

    return;
}
//****************************************************************************80

double polygon_area(int n, double x[], double y[])

//****************************************************************************80
//
//  Purpose:
//
//    POLYGON_AREA returns the area of a polygon.
//
//  Discussion:
//
//    The vertices should be listed in counter-clockwise order so that
//    the area will be positive.
//
//  Licensing:
//
//    This code is distributed under the GNU LGPL license.
//
//  Modified:
//
//    10 September 2016
//
//  Author:
//
//    John Burkardt.
//
//  Parameters:
//
//    Input, int N, the number of vertices.
//
//    Input, double X[N], Y[N], the vertex coordinates.
//
//    Output, double POLYGON_AREA, the area of the polygon.
//
{
    double area;
    int i;
    int im1;

    area = 0.0;
    im1 = n - 1;
    for (i = 0; i < n; i++) {
        area = area + x[im1] * y[i] - x[i] * y[im1];
        im1 = i;
    }
    area = 0.5 * area;

    return area;
}
//****************************************************************************80

int *polygon_triangulate(int n, double x[], double y[])

//****************************************************************************80
//
//  Purpose:
//
//    POLYGON_TRIANGULATE determines a triangulation of a polygon.
//
//  Discussion:
//
//    There are N-3 triangles in the triangulation.
//
//    For the first N-2 triangles, the first edge listed is always an
//    internal diagonal.
//
//    Thanks to Gene Dial for pointing out a mistake in the area calculation,
//    10 September 2016.
//
//    Gene Dial requested an angle tolerance of about 1 millionth radian or
//    5.7E-05 degrees, 26 June 2018.
//
//  Licensing:
//
//    This code is distributed under the GNU LGPL license.
//
//  Modified:
//
//    26 June 2018
//
//  Author:
//
//    Original C version by Joseph ORourke.
//    This C++ version by John Burkardt.
//
//  Reference:
//
//    Joseph ORourke,
//    Computational Geometry in C,
//    Cambridge, 1998,
//    ISBN: 0521649765,
//    LC: QA448.D38.
//
//  Parameters:
//
//    Input, int N, the number of vertices.
//
//    Input, double X[N], Y[N], the coordinates of each vertex.
//
//    Output, int TRIANGLES[3*(N-2)], the triangles of the
//    triangulation.
//
{
    double angle;
    const double angle_tol = 5.7E-05;
    double area;
    bool *ear;
    int i;
    int i0;
    int i1;
    int i2;
    int i3;
    int i4;
    int *next_node;
    int node;
    int node1;
    int node2;
    int node3;
    int node_m1;
    int *prev_node;
    int triangle_num;
    int *triangles;
    //
    //  We must have at least 3 vertices.
    //
    if (n < 3) {
        cerr << "\n";
        cerr << "POLYGON_TRIANGULATE - Fatal error!\n";
        cerr << "  N < 3.\n";
        exit(1);
    }
    //
    //  Consecutive vertices cannot be equal.
    //
    node_m1 = n - 1;
    for (node = 0; node < n; node++) {
        if (x[node_m1] == x[node] && y[node_m1] == y[node]) {
            cerr << "\n";
            cerr << "POLYGON_TRIANGULATE - Fatal error!\n";
            cerr << "  Two consecutive nodes are identical.\n";
            exit(1);
        }
        node_m1 = node;
    }
    //
    //  No node can be the vertex of an angle less than 1 degree
    //  in absolute value.
    //
    node1 = n - 1;

    for (node2 = 0; node2 < n; node2++) {
        node3 = ((node2 + 1) % n);

        angle = angle_degree(x[node1], y[node1], x[node2], y[node2], x[node3], y[node3]);

        if (fabs(angle) <= angle_tol) {
            cerr << "\n";
            cerr << "POLYGON_TRIANGULATE - Fatal error!\n";
            cerr << "  Polygon has an angle smaller than " << angle_tol << "\n";
            cerr << "  occurring at node " << node2 << "\n";
            exit(1);
        }
        node1 = node2;
    }
    //
    //  Area must be positive.
    //
    area = polygon_area(n, x, y);

    if (area <= 0.0) {
        cerr << "\n";
        cerr << "POLYGON_TRIANGULATE - Fatal error!\n";
        cerr << "  Polygon has zero or negative area.\n";
        exit(1);
    }

    triangles = new int[3 * (n - 2)];
    //
    //  PREV_NODE and NEXT_NODE point to the previous and next nodes.
    //
    prev_node = new int[n];
    next_node = new int[n];

    i = 0;
    prev_node[i] = n - 1;
    next_node[i] = i + 1;

    for (i = 1; i < n - 1; i++) {
        prev_node[i] = i - 1;
        next_node[i] = i + 1;
    }

    i = n - 1;
    prev_node[i] = i - 1;
    next_node[i] = 0;
    //
    //  EAR indicates whether the node and its immediate neighbors form an ear
    //  that can be sliced off immediately.
    //
    ear = new bool[n];
    for (i = 0; i < n; i++) {
        ear[i] = diagonal(prev_node[i], next_node[i], n, prev_node, next_node, x, y);
    }

    triangle_num = 0;

    i2 = 0;

    while (triangle_num < n - 3) {
        //
        //  If I2 is an ear, gather information necessary to carry out
        //  the slicing operation and subsequent "healing".
        //
        if (ear[i2]) {
            i3 = next_node[i2];
            i4 = next_node[i3];
            i1 = prev_node[i2];
            i0 = prev_node[i1];
            //
            //  Make vertex I2 disappear.
            //
            next_node[i1] = i3;
            prev_node[i3] = i1;
            //
            //  Update the earity of I1 and I3, because I2 disappeared.
            //
            ear[i1] = diagonal(i0, i3, n, prev_node, next_node, x, y);
            ear[i3] = diagonal(i1, i4, n, prev_node, next_node, x, y);
            //
            //  Add the diagonal [I3, I1, I2] to the list.
            //
            triangles[0 + triangle_num * 3] = i3;
            triangles[1 + triangle_num * 3] = i1;
            triangles[2 + triangle_num * 3] = i2;
            triangle_num = triangle_num + 1;
        }
        //
        //  Try the next vertex.
        //
        i2 = next_node[i2];
    }
    //
    //  The last triangle is formed from the three remaining vertices.
    //
    i3 = next_node[i2];
    i1 = prev_node[i2];

    triangles[0 + triangle_num * 3] = i3;
    triangles[1 + triangle_num * 3] = i1;
    triangles[2 + triangle_num * 3] = i2;
    triangle_num = triangle_num + 1;
    //
    //  Free memory.
    //
    delete[] ear;
    delete[] next_node;
    delete[] prev_node;

    return triangles;
}
//****************************************************************************80

double *r8mat_data_read(string input_filename, int m, int n)

//****************************************************************************80
//
//  Purpose:
//
//    R8MAT_DATA_READ reads the data from an R8MAT file.
//
//  Discussion:
//
//    An R8MAT is an array of R8's.
//
//    The file is assumed to contain one record per line.
//
//    Records beginning with '#' are comments, and are ignored.
//    Blank lines are also ignored.
//
//    Each line that is not ignored is assumed to contain exactly (or at least)
//    M real numbers, representing the coordinates of a point.
//
//    There are assumed to be exactly (or at least) N such records.
//
//  Licensing:
//
//    This code is distributed under the GNU LGPL license.
//
//  Modified:
//
//    23 February 2009
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, string INPUT_FILENAME, the name of the input file.
//
//    Input, int M, the number of spatial dimensions.
//
//    Input, int N, the number of points.  The program
//    will stop reading data once N values have been read.
//
//    Output, double R8MAT_DATA_READ[M*N], the data.
//
{
    bool error;
    ifstream input;
    int i;
    int j;
    string line;
    double *table;
    double *x;

    input.open(input_filename.c_str());

    if (!input) {
        cerr << "\n";
        cerr << "R8MAT_DATA_READ - Fatal error!\n";
        cerr << "  Could not open the input file: \"" << input_filename << "\"\n";
        exit(1);
    }

    table = new double[m * n];

    x = new double[m];

    j = 0;

    while (j < n) {
        getline(input, line);

        if (input.eof()) {
            break;
        }

        if (line[0] == '#' || s_len_trim(line) == 0) {
            continue;
        }

        error = s_to_r8vec(line, m, x);

        if (error) {
            continue;
        }

        for (i = 0; i < m; i++) {
            table[i + j * m] = x[i];
        }
        j = j + 1;
    }

    input.close();

    delete[] x;

    return table;
}
//****************************************************************************80

void r8mat_header_read(string input_filename, int &m, int &n)

//****************************************************************************80
//
//  Purpose:
//
//    R8MAT_HEADER_READ reads the header from an R8MAT file.
//
//  Discussion:
//
//    An R8MAT is an array of R8's.
//
//  Licensing:
//
//    This code is distributed under the GNU LGPL license.
//
//  Modified:
//
//    23 February 2009
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, string INPUT_FILENAME, the name of the input file.
//
//    Output, int &M, the number of spatial dimensions.
//
//    Output, int &N, the number of points.
//
{
    m = file_column_count(input_filename);

    if (m <= 0) {
        cerr << "\n";
        cerr << "R8MAT_HEADER_READ - Fatal error!\n";
        cerr << "  FILE_COLUMN_COUNT failed.\n";
        exit(1);
    }

    n = file_row_count(input_filename);

    if (n <= 0) {
        cerr << "\n";
        cerr << "R8MAT_HEADER_READ - Fatal error!\n";
        cerr << "  FILE_ROW_COUNT failed.\n";
        exit(1);
    }

    return;
}
//****************************************************************************80

int s_len_trim(string s)

//****************************************************************************80
//
//  Purpose:
//
//    S_LEN_TRIM returns the length of a string to the last nonblank.
//
//  Licensing:
//
//    This code is distributed under the GNU LGPL license.
//
//  Modified:
//
//    05 July 2009
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, string S, a string.
//
//    Output, int S_LEN_TRIM, the length of the string to the last nonblank.
//    If S_LEN_TRIM is 0, then the string is entirely blank.
//
{
    int n;

    n = s.length();

    while (0 < n) {
        if (s[n - 1] != ' ') {
            return n;
        }
        n = n - 1;
    }

    return n;
}
//****************************************************************************80

double s_to_r8(string s, int *lchar, bool *error)

//****************************************************************************80
//
//  Purpose:
//
//    S_TO_R8 reads an R8 from a string.
//
//  Discussion:
//
//    This routine will read as many characters as possible until it reaches
//    the end of the string, or encounters a character which cannot be
//    part of the real number.
//
//    Legal input is:
//
//       1 blanks,
//       2 '+' or '-' sign,
//       2.5 spaces
//       3 integer part,
//       4 decimal point,
//       5 fraction part,
//       6 'E' or 'e' or 'D' or 'd', exponent marker,
//       7 exponent sign,
//       8 exponent integer part,
//       9 exponent decimal point,
//      10 exponent fraction part,
//      11 blanks,
//      12 final comma or semicolon.
//
//    with most quantities optional.
//
//  Example:
//
//    S                 R
//
//    '1'               1.0
//    '     1   '       1.0
//    '1A'              1.0
//    '12,34,56'        12.0
//    '  34 7'          34.0
//    '-1E2ABCD'        -100.0
//    '-1X2ABCD'        -1.0
//    ' 2E-1'           0.2
//    '23.45'           23.45
//    '-4.2E+2'         -420.0
//    '17d2'            1700.0
//    '-14e-2'         -0.14
//    'e2'              100.0
//    '-12.73e-9.23'   -12.73 * 10.0^(-9.23)
//
//  Licensing:
//
//    This code is distributed under the GNU LGPL license.
//
//  Modified:
//
//    05 July 2009
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, string S, the string containing the
//    data to be read.  Reading will begin at position 1 and
//    terminate at the end of the string, or when no more
//    characters can be read to form a legal real.  Blanks,
//    commas, or other nonnumeric data will, in particular,
//    cause the conversion to halt.
//
//    Output, int *LCHAR, the number of characters read from
//    the string to form the number, including any terminating
//    characters such as a trailing comma or blanks.
//
//    Output, bool *ERROR, is true if an error occurred.
//
//    Output, double S_TO_R8, the real value that was read from the string.
//
{
    char c;
    int ihave;
    int isgn;
    int iterm;
    int jbot;
    int jsgn;
    int jtop;
    int nchar;
    int ndig;
    double r;
    double rbot;
    double rexp;
    double rtop;
    char TAB = 9;

    nchar = s_len_trim(s);
    *error = false;
    r = 0.0;
    *lchar = -1;
    isgn = 1;
    rtop = 0.0;
    rbot = 1.0;
    jsgn = 1;
    jtop = 0;
    jbot = 1;
    ihave = 1;
    iterm = 0;

    for (;;) {
        c = s[*lchar + 1];
        *lchar = *lchar + 1;
        //
        //  Blank or TAB character.
        //
        if (c == ' ' || c == TAB) {
            if (ihave == 2) {
            } else if (ihave == 6 || ihave == 7) {
                iterm = 1;
            } else if (1 < ihave) {
                ihave = 11;
            }
        }
        //
        //  Comma.
        //
        else if (c == ',' || c == ';') {
            if (ihave != 1) {
                iterm = 1;
                ihave = 12;
                *lchar = *lchar + 1;
            }
        }
        //
        //  Minus sign.
        //
        else if (c == '-') {
            if (ihave == 1) {
                ihave = 2;
                isgn = -1;
            } else if (ihave == 6) {
                ihave = 7;
                jsgn = -1;
            } else {
                iterm = 1;
            }
        }
        //
        //  Plus sign.
        //
        else if (c == '+') {
            if (ihave == 1) {
                ihave = 2;
            } else if (ihave == 6) {
                ihave = 7;
            } else {
                iterm = 1;
            }
        }
        //
        //  Decimal point.
        //
        else if (c == '.') {
            if (ihave < 4) {
                ihave = 4;
            } else if (6 <= ihave && ihave <= 8) {
                ihave = 9;
            } else {
                iterm = 1;
            }
        }
        //
        //  Exponent marker.
        //
        else if (ch_eqi(c, 'E') || ch_eqi(c, 'D')) {
            if (ihave < 6) {
                ihave = 6;
            } else {
                iterm = 1;
            }
        }
        //
        //  Digit.
        //
        else if (ihave < 11 && '0' <= c && c <= '9') {
            if (ihave <= 2) {
                ihave = 3;
            } else if (ihave == 4) {
                ihave = 5;
            } else if (ihave == 6 || ihave == 7) {
                ihave = 8;
            } else if (ihave == 9) {
                ihave = 10;
            }

            ndig = ch_to_digit(c);

            if (ihave == 3) {
                rtop = 10.0 * rtop + (double)ndig;
            } else if (ihave == 5) {
                rtop = 10.0 * rtop + (double)ndig;
                rbot = 10.0 * rbot;
            } else if (ihave == 8) {
                jtop = 10 * jtop + ndig;
            } else if (ihave == 10) {
                jtop = 10 * jtop + ndig;
                jbot = 10 * jbot;
            }

        }
        //
        //  Anything else is regarded as a terminator.
        //
        else {
            iterm = 1;
        }
        //
        //  If we haven't seen a terminator, and we haven't examined the
        //  entire string, go get the next character.
        //
        if (iterm == 1 || nchar <= *lchar + 1) {
            break;
        }
    }
    //
    //  If we haven't seen a terminator, and we have examined the
    //  entire string, then we're done, and LCHAR is equal to NCHAR.
    //
    if (iterm != 1 && (*lchar) + 1 == nchar) {
        *lchar = nchar;
    }
    //
    //  Number seems to have terminated.  Have we got a legal number?
    //  Not if we terminated in states 1, 2, 6 or 7!
    //
    if (ihave == 1 || ihave == 2 || ihave == 6 || ihave == 7) {
        *error = true;
        return r;
    }
    //
    //  Number seems OK.  Form it.
    //
    if (jtop == 0) {
        rexp = 1.0;
    } else {
        if (jbot == 1) {
            rexp = pow(10.0, jsgn * jtop);
        } else {
            rexp = jsgn * jtop;
            rexp = rexp / jbot;
            rexp = pow(10.0, rexp);
        }
    }

    r = isgn * rexp * rtop / rbot;

    return r;
}
//****************************************************************************80

bool s_to_r8vec(string s, int n, double rvec[])

//****************************************************************************80
//
//  Purpose:
//
//    S_TO_R8VEC reads an R8VEC from a string.
//
//  Licensing:
//
//    This code is distributed under the GNU LGPL license.
//
//  Modified:
//
//    05 July 2009
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, string S, the string to be read.
//
//    Input, int N, the number of values expected.
//
//    Output, double RVEC[N], the values read from the string.
//
//    Output, bool S_TO_R8VEC, is true if an error occurred.
//
{
    int begin;
    bool error;
    int i;
    int lchar;
    int length;

    begin = 0;
    length = s.length();
    error = 0;

    for (i = 0; i < n; i++) {
        rvec[i] = s_to_r8(s.substr(begin, length), &lchar, &error);

        if (error) {
            return error;
        }
        begin = begin + lchar;
        length = length - lchar;
    }

    return error;
}
//****************************************************************************80

int s_word_count(string s)

//****************************************************************************80
//
//  Purpose:
//
//    S_WORD_COUNT counts the number of "words" in a string.
//
//  Licensing:
//
//    This code is distributed under the GNU LGPL license.
//
//  Modified:
//
//    05 July 2009
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, string S, the string to be examined.
//
//    Output, int S_WORD_COUNT, the number of "words" in the string.
//    Words are presumed to be separated by one or more blanks.
//
{
    bool blank;
    int char_count;
    int i;
    int word_count;

    word_count = 0;
    blank = true;

    char_count = s.length();

    for (i = 0; i < char_count; i++) {
        if (isspace(s[i])) {
            blank = true;
        } else if (blank) {
            word_count = word_count + 1;
            blank = false;
        }
    }

    return word_count;
}
//****************************************************************************80

void timestamp()

//****************************************************************************80
//
//  Purpose:
//
//    TIMESTAMP prints the current YMDHMS date as a time stamp.
//
//  Example:
//
//    31 May 2001 09:45:54 AM
//
//  Licensing:
//
//    This code is distributed under the GNU LGPL license.
//
//  Modified:
//
//    08 July 2009
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    None
//
{
#define TIME_SIZE 40

    static char time_buffer[TIME_SIZE];
    const struct std::tm *tm_ptr;
    std::time_t now;

    now = std::time(NULL);
    tm_ptr = std::localtime(&now);

    std::strftime(time_buffer, TIME_SIZE, "%d %B %Y %I:%M:%S %p", tm_ptr);

    std::cout << time_buffer << "\n";

    return;
#undef TIME_SIZE
}
//****************************************************************************80

double triangle_area(double xa, double ya, double xb, double yb, double xc, double yc)

//****************************************************************************80
//
//  Purpose:
//
//    TRIANGLE_AREA computes the signed area of a triangle.
//
//  Licensing:
//
//    This code is distributed under the GNU LGPL license.
//
//  Modified:
//
//    06 May 2014
//
//  Author:
//
//    John Burkardt
//
//  Parameters:
//
//    Input, double XA, YA, XB, YB, XC, YC, the coordinates of
//    the vertices of the triangle, given in counterclockwise order.
//
//    Output, double TRIANGLE_AREA, the signed area of the triangle.
//
{
    double value;

    value = 0.5 * ((xb - xa) * (yc - ya) - (xc - xa) * (yb - ya));

    return value;
}
