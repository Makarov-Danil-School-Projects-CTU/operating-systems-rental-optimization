📝 Task Objective

Your task is to implement a set of classes that will optimize the profit for rental services.

We assume that we operate a tool rental service. The rental service has tools available in various quantities, e.g., 1x excavator, 3x vibratory rammers, etc. The rental service has a website where customers can submit their rental requests. The rental service uses innovative business models, so it rents out its tools through an auction system. Customers submit their rental requests, and each request includes the rental period (interval from-to) and the offered payment for the rental. The rental service then selects from the submitted intervals to achieve maximum profit. The following constraints must be observed when renting:

🔢 Tool Availability

At any given time, we can only serve as many customers as the number of available tools of a given type.

⏳ Non-Overlapping Intervals

The intervals of consecutive rentals of individual tools must not overlap or touch.

🔄 Tool Limit

Our rental service isn't very wealthy, so we have at most 3 units of each tool type.
The above problem isn't as trivial as it may initially seem. The solution can be found using dynamic programming; the algorithm was explained in the last BI-AG1 lecture. For 1, 2, and 3 units of a rental tool, the problem can be solved in O(n log(n)), O(n²), and O(n³) times, respectively. Since this is a problem with higher computational complexity, it is worth utilizing multiple threads to speed up the computation.

📊 Example Input Intervals

From To Offered Payment
0 6 8

0	6	8

7	9	2

10	13	2

0	1	4

2	11	4

12	13	4

0	3	2

4	5	2

8	13	8

📈 Sample Rental Plans and Profits

If renting 1 unit of the tool:

Rental Plan: 1: 0-6 8-13

Profit: 16

If renting 2 units of the tool:

Rental Plan:

1: 0-6 7-9 12-13

2: 0-1 4-5 8-13

Profit: 28

If renting 3 units of the tool:

Rental Plan:

1: 0-6 7-9 10-13

2: 0-1 2-11 12-13

3: 0-3 4-5 8-13

Profit: 36

🛠️ Implementation Requirements

The expected solution must correctly integrate into the infrastructure described below and must properly create, schedule, synchronize, and terminate threads.
Implementing the algorithmic solution to the problem is not strictly necessary; the testing environment offers an interface capable of solving the problem sequentially.
