#include "LinKerninghan.h"

Tour::Tour(vi &s, int V, double objValue)
{
    tour = s;
    tour.pop_back();
    cost = objValue;
    N = V;
    inv.assign(N + 1, 0);
    for (int i = 0; i < V; i++)
    {
        inv[tour[i]] = i;
    }
    reversed = false;
}

void Tour::print()
{
    for (int i = 0; i < tour.size(); i++)
    {
        std::cout << tour[i] << " ";
    }
    std::cout << "\n";
    // std::cout << tour[0] << " \n";
}

double Tour::getCost() { return cost; }
int Tour::inverse(int node) { return inv[node]; }
int Tour::next(int node)
{
    if (!reversed)
        return tour[(inv[node] + 1) % N];
    return tour[((inv[node] - 1) % N + N) % N];
}
int Tour::prev(int node)
{
    if (!reversed)
        return tour[((inv[node] - 1) % N + N) % N];
    return tour[(inv[node] + 1) % N];
}
int Tour::getN() { return N; }
vi Tour::getTour()
{
    return tour;
}

void Tour::flip(int a, int b)
{
    int segment_size = abs(inv[b] - inv[a]) + 1;
    cout << segment_size << "\n";
    if (segment_size > N / 2)
    {
        a = prev(a);
        b = next(b);
        reversed = !reversed;
        segment_size = N - segment_size;
    }

    for (int i = 0; i < segment_size / 2; i++)
    {
        // cout << tour[inv[a]] << " " << tour[inv[b]] << "\n";
        swap(tour[inv[a]], tour[inv[b]]);
        int x = a, y = b;

        a = next(a);
        b = prev(b);

        swap(inv[x], inv[y]);
    }

    // if (x > y)
    //     swap(x, y);

    // if (abs(y - x) > N / 2)
    // {
    //     x = inv[prev(a)];
    //     y = inv[next(b)];
    //     if (x > y)
    //         swap(x, y);
    //     reversed = !reversed;

    //     for (int i = x, j = y; i >= 0 && j < N; i--, j++)
    //     {
    //         swap(tour[i], tour[j]);
    //         swap(inv[tour[i]], inv[tour[j]]);
    //     }

    //     return;
    // }
    // cout << x << " " << y << "\n";

    // for (int i = x, j = y; i < j; i++, j--)
    // {
    //     swap(tour[i], tour[j]);
    //     swap(inv[tour[i]], inv[tour[j]]);
    // }
}
bool Tour::sequence(int a, int b, int c)
{
    if (inv[a] < inv[b] && inv[b] < inv[c] || inv[c] < inv[b] && inv[b] < inv[a])
        return true;

    return false;
}

void lkStep(Tour &T, double **c, vector<vector<int>> neighbourSet)
{
    double delta = 0;

    T.print();

    int base = 1, a;
    vector<bool> taken;
    taken.assign(T.getN() + 1, false);

    while ((a = findPromisingVertex(T, c, base, delta, taken, neighbourSet)) != -1)
    {
        delta += c[base][T.next(base)] - c[T.next(base)][a] + c[T.prev(a)][a] - c[T.prev(a)][base];
        cout << "\n"
             << T.next(base) << " " << T.prev(a) << " " << T.getCost() - delta << "\n";

        T.flip(T.next(base), T.prev(a));
        T.print();

        vi s = T.getTour();
        for(int i = 1; i <=T.getN();i++)
        {
            cout << T.inverse(i) << " ";
        }
        std::cout << "\n";
        // printSolucao(s);
        cout << calcularValorObj(s, c) + c[s[s.size() - 1]][s[0]] << "\n";
    }
}

int findPromisingVertex(Tour &T, double **c, int base, double delta, vector<bool> &taken, vector<vector<int>> neighbourSet)
{
    double A = delta + c[base][T.next(base)];

    pair<double, int> best_a = {-INF, -1};

    int k = 5;

    for (int i = 1; i <= k; i++)
    {
        int a = neighbourSet[T.next(base) - 1][i];
        pair<double, int> cost = {c[T.prev(a)][a] - c[T.next(base)][a], a};
        if (taken[a] == false && cost > best_a && a != base && a != T.next(base) && a != T.prev(base))
        {
            best_a = cost;
        }
    }

    if (best_a.second != -1)
    {
        taken[best_a.second] = true;
    }

    return best_a.second;

    // for (int probe = 1; probe <= T.getN(); probe++)
    // {
    //     if (taken[probe] == false && A > c[T.next(base)][T.next(probe)] && probe != base && probe != T.next(base) && probe != T.prev(base))
    //     {
    //         taken[probe] = true;
    //         return probe;
    //     }
    // }
    // return -1;
}