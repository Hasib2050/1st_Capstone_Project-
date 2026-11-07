//Code-01
#include <stdio.h>

struct Student {
    int age;
    float marks;
};

int main() {

    struct Student s = {20, 85.5};

    // Accessing members using dot operator
    printf("Age: %d\n", s.age);
    printf("Marks: %.1f\n", s.marks);

    // Creating a pointer to the structure
    struct Student *ptr = &s;

    // Accessing members using arrow operator
    printf("Age using pointer: %d\n", ptr->age);
    printf("Marks using pointer: %.1f", ptr->marks);


}
//Code-02
#include <stdio.h>

// Defining a structure to represent a student
struct Student
{
    char name[50];
    int age;
    float grade;
};

int main()
{

    // Declaring and initializing a structure variable
    struct Student s1 = {"Rahul", 20, 18.5};

    // Designated Initializing another structure
    struct Student s2 = {.age = 18, .name = "Vikas", .grade = 22};

    // Accessing structure members
    printf("%s\t%d\t%.2f\n", s1.name, s1.age, s1.grade);
    printf("%s\t%d\t%.2f\n", s2.name, s2.age, s2.grade);

}
//Code-03
#include <stdio.h>
#include <stdlib.h>

struct Student {
    int id;
  	char grade;
};

int main() {
    struct Student s1 = {1, 'A'};

  	// Create a copy of student s1
  	struct Student s1c = s1;

    printf("Student 1 ID: %d\n", s1c.id);
    printf("Student 1 Grade: %c", s1c.grade);
  	return 0;
}
//Code-04
#include <stdio.h>

// Structure definition
struct A {
    int x;
};

// Function to increment values
void increment(struct A a, struct A *b) {
    a.x++;
  	b->x++;
}

int main() {
    struct A a  = { 10 };
  	struct A b  = { 10 };

  	// Passing a by value and b by pointer
  	increment(a, &b);

  	printf("a.x: %d \tb.x: %d", a.x, b.x);
    return 0;
}
//Code-05
#include <stdio.h>

// Defining structure
typedef struct {
    int a;
} str1;

// Another way of using typedef with structures
typedef struct {
    int x;
} str2;

int main() {

    // Creating structure variables using new names
    str1 var1 = { 20 };
    str2 var2 = { 314 };

    printf("var1.a = %d\n", var1.a);
    printf("var2.x = %d\n", var2.x);
    return 0;
}
//Code-06
#include <stdio.h>

// Child structure declaration
struct child {
    int x;
    char c;
};

// Parent structure declaration
struct parent {
    int a;
    struct child b;
};

int main() {
    struct parent p = { 25, 195, 'A' };

    // Accessing and printing nested members
    printf("p.a = %d\n", p.a);
    printf("p.b.x = %d\n", p.b.x);
    printf("p.b.c = %c", p.b.c);
    return 0;
}
