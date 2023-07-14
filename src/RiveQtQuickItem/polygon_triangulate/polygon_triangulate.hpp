// SPDX-FileCopyrightText: 2016 John Burkardt https://people.sc.fsu.edu/~jburkardt/cpp_src/polygon_triangulate/polygon_triangulate.html
//
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <string>

double angle_degree(double x1, double y1, double x2, double y2, double x3, double y3);
bool between(double xa, double ya, double xb, double yb, double xc, double yc);
char ch_cap(char ch);
bool ch_eqi(char ch1, char ch2);
int ch_to_digit(char ch);
bool collinear(double xa, double ya, double xb, double yb, double xc, double yc);
bool diagonal(int im1, int ip1, int n, int prev[], int next[], double x[], double y[]);
bool diagonalie(int im1, int ip1, int n, int next[], double x[], double y[]);
int file_column_count(std::string input_filename);
int file_row_count(std::string input_filename);
void i4mat_transpose_print(int m, int n, int a[], std::string title);
void i4mat_transpose_print_some(int m, int n, int a[], int ilo, int jlo, int ihi, int jhi, std::string title);
void i4mat_write(std::string output_filename, int m, int n, int table[]);
bool in_cone(int im1, int ip1, int n, int prev[], int next[], double x[], double y[]);
bool intersect(double xa, double ya, double xb, double yb, double xc, double yc, double xd, double yd);
bool intersect_prop(double xa, double ya, double xb, double yb, double xc, double yc, double xd, double yd);
bool l4_xor(bool l1, bool l2);
void l4vec_print(int n, bool a[], std::string title);
double polygon_area(int n, double x[], double y[]);

int *polygon_triangulate(int n, double x[], double y[]);

double *r8mat_data_read(std::string input_filename, int m, int n);
void r8mat_header_read(std::string input_filename, int &m, int &n);
int s_len_trim(std::string s);
double s_to_r8(std::string s, int &lchar, int &error);
bool s_to_r8vec(std::string s, int n, double rvec[]);
int s_word_count(std::string s);
void timestamp();
double triangle_area(double xa, double ya, double xb, double yb, double xc, double yc);
