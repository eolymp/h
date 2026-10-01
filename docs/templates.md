# Function problems and code templates

On a `FUNCTION` problem the contestant writes one function instead of a program. Each runtime
the problem accepts has a code template with three parts:

| Part | Shown to the contestant | Is |
| --- | --- | --- |
| header | no | what comes before the submission: includes, imports, declarations |
| source | yes, in the editor | the stub the contestant starts from |
| footer | no | what comes after it: reading the input, calling the function, printing the result |

The judge compiles the header, the submission and the footer, in that order, as one file, and
runs it as an ordinary program, so the checker, the tests and the scoring are a `PROGRAM`
problem's. A template is the problem's own code that every contestant compiles; nothing
graded belongs in it, since in some languages a submission can read its own header, footer
and files, so correctness stays in the checker. The judge keeps one template per runtime, and the header and
the footer are editable only on a `FUNCTION` problem.

## C++

The header declares the function and ends with a `#line` directive, so the compiler counts
the contestant's lines from 1, in a file called `solution.cpp`:

```cpp
#include <cstdio>
#include <vector>
long long max_pair_sum(const std::vector<int>& a);
#line 1 "solution.cpp"
```

The stub is what the editor shows:

```cpp
long long max_pair_sum(const std::vector<int>& a) {
    return 0;
}
```

The footer starts with an empty line, so a submission that does not end in a line break does
not run into it, and then with its own `#line`, so an error in the footer is not blamed on the
contestant's line numbers:

```c++

#line 1 "grader.cpp"
int main() {
    int n;
    if (std::scanf("%d", &n) != 1) return 1;
    std::vector<int> a(n);
    for (int& v : a) if (std::scanf("%d", &v) != 1) return 1;
    std::printf("%lld\n", max_pair_sum(a));
}
```

What the judge compiles for a correct submission is then this one file:

```cpp
#include <cstdio>
#include <vector>
long long max_pair_sum(const std::vector<int>& a);
#line 1 "solution.cpp"
long long max_pair_sum(const std::vector<int>& a) {
    long long first = a[0] > a[1] ? a[0] : a[1];
    long long second = a[0] > a[1] ? a[1] : a[0];
    for (std::size_t at = 2; at < a.size(); at++) {
        if (a[at] > first) {
            second = first;
            first = a[at];
        } else if (a[at] > second) {
            second = a[at];
        }
    }
    return first + second;
}

#line 1 "grader.cpp"
int main() {
    int n;
    if (std::scanf("%d", &n) != 1) return 1;
    std::vector<int> a(n);
    for (int& v : a) if (std::scanf("%d", &v) != 1) return 1;
    std::printf("%lld\n", max_pair_sum(a));
}
```

Use the standard headers rather than `<bits/stdc++.h>`, which only GCC's library has. A
submission may include what it needs itself; the header's `#line` does not stop it.

## Python

The header imports what the footer uses, and the footer calls the function under
`if __name__ == "__main__":`. Python reports errors by line in the one file it was given, so
there is no `#line`; a comment in the header marks where the contestant's code starts.

```python
import sys
# ---- contestant code below ----
```

```python
def max_pair_sum(a):
    return 0
```

```python

# ---- grader ----
if __name__ == "__main__":
    _d = sys.stdin.read().split()
    _n = int(_d[0])
    print(max_pair_sum([int(v) for v in _d[1:1 + _n]]))
```

Names the footer uses for itself start with `_`, so they do not collide with the contestant's.

## Java

The class is opened in the header and closed in the footer, so the contestant writes a
`static` method inside `Main`, and `main` is the footer's:

```java
import java.util.*;
import java.io.*;

public class Main {
```

```java
static long maxPairSum(int[] a) {
    return 0;
}
```

```java

public static void main(String[] args) throws IOException {
    StreamTokenizer in = new StreamTokenizer(new BufferedReader(new InputStreamReader(System.in)));
    in.nextToken(); int n = (int) in.nval;
    int[] a = new int[n];
    for (int i = 0; i < n; i++) { in.nextToken(); a[i] = (int) in.nval; }
    System.out.println(maxPairSum(a));
}
}
```

## A submission with its own `main`

The footer holds `main`, so a contestant who submits a whole program, as on a `PROGRAM`
problem, defines it twice and gets a compilation error, here in GCC's words:

```
grader.cpp:1:5: error: redefinition of 'int main()'
solution.cpp:8:5: note: 'int main()' previously defined here
```

In Java the second `main` is a duplicate method of `Main`, also a compilation error. Python
has no compilation step: a submission that reads the input itself runs its own code first,
top to bottom, and then the footer's, which finds the input already read. Say in the statement that
the contestant writes only the function, and show its signature for every language.

## Running one locally

`eo-judge` builds a `FUNCTION` problem's C++ solutions inside their template exactly as the
judge concatenates them, and `check` warns about a C++ header that does not end with its
`#line` and a line break, or a footer that does not start on a new line (EO913), a stub that
does not compile or is judged as anything but a wrong answer (EO823), and a template in which a
whole program compiles (EO824).
[judge.md](judge.md#function-problems) has the `problem.json` fields.
