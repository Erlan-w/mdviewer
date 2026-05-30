# Markdown Viewer — Feature Demo

Welcome to the **Markdown Viewer** demo. This file exercises all rendering features.

---

## Typography

Paragraphs render with proper **bold**, *italic*, ***bold italic***, and `inline code` styling.

Link example: [Visit Qt Documentation](https://doc.qt.io)

> This is a blockquote.
> It supports *italic* and **bold** text inside.
> Used for notes, warnings, or citations.

---

## Code Blocks

### Python

```python
def fibonacci(n: int) -> list[int]:
    """Generate Fibonacci sequence."""
    seq = [0, 1]
    for _ in range(n - 2):
        seq.append(seq[-1] + seq[-2])
    return seq

# Example usage
result = fibonacci(10)
print(f"First 10: {result}")
```

### C++

```cpp
#include <iostream>
#include <vector>

template<typename T>
class Stack {
public:
    void push(T val) { data.push_back(val); }
    T pop() {
        T val = data.back();
        data.pop_back();
        return val;
    }
private:
    std::vector<T> data;
};

int main() {
    Stack<int> s;
    s.push(42);
    std::cout << s.pop() << std::endl;
    return 0;
}
```

### JavaScript

```javascript
const fetchData = async (url) => {
    try {
        const response = await fetch(url);
        const data = await response.json();
        return data;
    } catch (error) {
        console.error('Fetch failed:', error);
        return null;
    }
};
```

---

## Charts

Bar chart example (click to open fullscreen, zoom and drag):

```chart
type:bar
title:Monthly Sales 2024
labels:Jan,Feb,Mar,Apr,May,Jun
data:120,145,98,172,163,189
legend:Revenue ($K)
```

Line chart:

```chart
type:line
title:CPU Usage Over Time
labels:0s,10s,20s,30s,40s,50s
data:45,62,78,55,43,71
legend:CPU %
```

Pie chart:

```chart
type:pie
title:Market Share
labels:Product A,Product B,Product C,Other
data:35,28,22,15
```

---

## Tables

| Feature           | Status  | Notes                     |
|-------------------|---------|---------------------------|
| Markdown parsing  | ✅ Done  | Full CommonMark subset    |
| Syntax highlight  | ✅ Done  | Python, C++, JS/TS        |
| Charts            | ✅ Done  | Bar, Line, Pie            |
| Image rendering   | ✅ Done  | Local + remote            |
| TOC navigation    | ✅ Done  | Auto-generated            |
| File watching     | ✅ Done  | Auto-reload on save       |
| Drag & drop       | ✅ Done  | Drop .md files            |

---

## Lists

### Unordered

- First item with `inline code`
- Second item with **bold text**
- Third item with *italic text*

### Ordered

1. Configure Qt6 and CMake
2. Build with GitHub Actions CI/CD
3. Deploy artifacts to release

---

## Heading Levels

### H3 Heading
#### H4 Heading
##### H5 Heading
###### H6 Heading

---

## Images

Local image (replace with valid path):

![Sample diagram](./diagram.png)

Remote image:

![Qt Logo](https://upload.wikimedia.org/wikipedia/commons/thumb/0/0b/Qt_logo_2016.svg/200px-Qt_logo_2016.svg.png)

---

*End of demo file.*
