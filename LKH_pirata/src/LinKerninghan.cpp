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
    cout << reversed << "\n";
    // std::cout << tour[0] << " \n";
}

double Tour::getCost() { return cost; }
void Tour::setCost(double c) { cost = c; }

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

int breadth(int k)
{
    return (k <= 2) ? 5 : 5;
}

void Tour::flip(int a, int b)
{
    int segment_size = abs(inv[a] - inv[b]) + 1;

    if (!(!reversed && (inv[a] < inv[b]) || reversed && (inv[a] > inv[b])))
    {
        a = prev(a);
        b = next(b);
        segment_size = abs(inv[a] - inv[b]) + 1;
        reversed = !reversed;
    }

    if (segment_size > N / 2)
    {
        a = prev(a);
        b = next(b);
        reversed = !reversed;
        segment_size = N - segment_size;
    }
    for (int i = 0; i < segment_size / 2; i++)
    {
        swap(tour[inv[a]], tour[inv[b]]);
        int x = a, y = b;

        a = next(a);
        b = prev(b);

        swap(inv[x], inv[y]);
    }
}
bool Tour::sequence(int a, int b, int c)
{
    if (!reversed)
    {
        if (inv[a] <= inv[b] && inv[b] <= inv[c])
            return true;
    }
    else
    {
        if (inv[c] <= inv[b] && inv[b] <= inv[a])
            return true;
    }
    return false;
}

void lkStep(Tour &T, double **c, vector<vector<int>> &neighbourSet)
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
             << T.next(base) << " " << a << " " << T.getCost() - delta << "\n";

        T.flip(T.next(base), T.prev(a));
        // T.print();

        vi s = T.getTour();
        // for (int i = 1; i <= T.getN(); i++)
        // {
        //     cout << T.inverse(i) << " ";
        // }
        std::cout << "\n";
        // printSolucao(s);
        cout << calcularValorObj(s, c) + c[s[s.size() - 1]][s[0]] << "\n";
    }
}

int findPromisingVertex(Tour &T, double **c, int base, double delta, vector<bool> &taken, vector<vector<int>> &neighbourSet)
{
    double A = delta + c[base][T.next(base)];

    pair<double, int> best_a = {-INF, -1};

    int k = 13;

    for (int i = 1; i <= k; i++)
    {
        int a = neighbourSet[T.next(base) - 1][i];
        pair<double, int> cost = {c[T.prev(a)][a] - c[T.next(base)][a], a};
        if (taken[a] == false && cost > best_a && T.prev(a) != base && T.prev(a) != T.next(base) && T.prev(a) != T.prev(base))
        {
            best_a = cost;
        }
    }

    if (best_a.second != -1)
    {
        taken[best_a.second] = true;
    }

    return best_a.second;
}

void step(Tour &T, double **c, int base, int level, float delta, double &final_delta, vector<vector<int>> &neighbourSet, deque<pair<pair<int, int>, double>> &flipSequence, vector<bool> &taken)
{
    // cout << T.getCost() - delta << " " << level << "\n";

    int k = breadth(level);

    // Create lk-ordering for base and next(base)
    vector<pair<pair<double, int>, bool>> lk_ordering;
    int lk_ordering_size = T.getN() - 3;
    lk_ordering.assign(2 * lk_ordering_size, {{0, 0}, 0});

    for (int i = 0, a = T.next(T.next(T.next(base))); i < lk_ordering_size; i++)
    {
        // cout << a << "\n";
        double greedy_cost = c[T.prev(a)][a] - c[T.next(base)][a];
        lk_ordering[i] = {{-greedy_cost, a}, false};
        a = T.next(a);
    }
    for (int i = lk_ordering_size, a = T.next(T.next(base)); i < 2 * lk_ordering_size; i++)
    {
        // cout << a << "\n";
        double greedy_cost = c[a][T.next(a)] - c[base][a];
        lk_ordering[i] = {{-greedy_cost, a}, true};
        a = T.next(a);
    }

    sort(lk_ordering.begin(), lk_ordering.end());

    for (int i = 0; i < k; i++)
    {
        int a = lk_ordering[i].first.second;

        if (taken[a] == true)
        {
            continue;
        }

        double g = 0;

        if (lk_ordering[i].second == false)
        {
            taken[a] = true;

            g = c[base][T.next(base)] - c[T.next(base)][a] + c[T.prev(a)][a] - c[T.prev(a)][base];

            flipSequence.push_back({{T.next(base), T.prev(a)}, g});
            final_delta += g;
            T.flip(T.next(base), T.prev(a));

            step(T, c, base, level + 1, delta + g, final_delta, neighbourSet, flipSequence, taken);
        }
        else
        {
            taken[a] = true;

            g = c[base][T.next(base)] - c[base][a] + c[a][T.next(a)] - c[T.next(a)][T.next(base)];

            flipSequence.push_back({{T.next(a), base}, g});
            final_delta += g;
            T.flip(T.next(a), base);

            step(T, c, base, level + 1, delta + g, final_delta, neighbourSet, flipSequence, taken);
        }

        if (delta > 0)
        {
            return;
        }
        else
        {
            if (!flipSequence.empty())
            {
                pair<pair<int, int>, double> fl = flipSequence.back();
                T.flip(fl.first.second, fl.first.first);
                final_delta -= fl.second;
                flipSequence.pop_back();
            }
        }
    }
}

void alternate_step(Tour &T, double **c, int base, int level, float delta, vector<vector<int>> &neighbourSet, deque<pair<int, int>> &flipSequence, vector<bool> &taken)
{
    // Create lk-ordering for base and next(base)
    vector<pair<double, int>> A_ordering;
    int A_ordering_size = T.getN() - 3;
    A_ordering.assign(2 * A_ordering_size, {0, 0});

    // for (int i = 0, a = T.next(T.next(T.next(base))); i < A_ordering_size; i++)
    // {
    //     // cout << a << "\n";
    //     double greedy_cost = c[T.prev(a)][a] - c[T.next(base)][a];
    //     A_ordering[i] = {{-greedy_cost, a}, false};
    //     a = T.next(a);
    // }

    int breadthA = 5;

    for (int i = 1, j = 0; i <= 14; i++)
    {
        int a = neighbourSet[T.next(base) - 1][i];
        double cost = delta + c[base][T.next(base)] - c[T.next(base)][a];

        if (cost > 0)
        {
            cout << "e\n";
            A_ordering[j] = {-cost, a};
            j++;
        }
    }
    sort(A_ordering.begin(), A_ordering.end());
}

bool lk_search(Tour &T, int v, double **c, vector<vector<int>> &neighbourSet, deque<pair<pair<int, int>, double>> &flipSequence)
{
    Tour current_tour = T;

    vector<bool> taken;
    taken.assign(T.getN() + 1, false);

    double delta = 0;

    step(current_tour, c, v, 1, 0, delta, neighbourSet, flipSequence, taken);
    current_tour.setCost(T.getCost() - delta);

    if (current_tour.getCost() < T.getCost())
    {
        // T.setCost(current_tour.getCost());
        return true;
    }
    else
    {
        return false;
    }
}

void lin_kerninghan(Tour &T, Tour &lk_tour, double **c, vector<vector<int>> &neighbourSet)
{
    lk_tour = T;
    vector<bool> marked;
    marked.assign(T.getN(), true);
    int markedVertices = T.getN() + 1;

    while (true)
    {
        int v;
        for (v = 1; v <= T.getN() && marked[v] == false; v++)
        {
        }

        if (v == 15)
            break;

        deque<pair<pair<int, int>, double>> flipSequence;

        bool flag = lk_search(lk_tour, v, c, neighbourSet, flipSequence);

        if (flag == true)
        {
            while (!flipSequence.empty())
            {
                int x = flipSequence.front().first.first;
                int y = flipSequence.front().first.second;
                double g = flipSequence.front().second;

                lk_tour.flip(x, y);
                lk_tour.setCost(lk_tour.getCost() - g);

                marked[x] = marked[y] = true;

                flipSequence.pop_front();
            }
        }
        else
        {
            marked[v] = false;
        }
    }

    return;
}

void kick(Tour &T)
{
    int t1, t2, t3, t4;

    
}