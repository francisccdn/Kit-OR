#include "Problema.h"

Problema::Problema(Data &d, double UB)
{
    this->data = d;
    this->bestInteger = UB;

    //Construçao do mestre

    env1 = IloEnv();
    masterModel = IloModel(env1);
    lambda = IloNumVarArray(env1, data.getNItems(), 0.0, IloInfinity);
    masterRanges = IloRangeArray(env1);

    IloExpr sum(env1);
    for (int i = 0; i < data.getNItems(); i++)
    {
        sum += M * lambda[i];
        masterRanges.add(lambda[i] == 1);
    }

    masterModel.add(masterRanges);
    masterObj = IloObjective(env1);
    masterObj = IloMinimize(env1, sum);
    masterModel.add(masterObj);

    master = IloCplex(masterModel);

    pi = IloNumArray(env1, data.getNItems());

    lambdaItens = std::vector<std::vector<bool>>(data.getNItems(), std::vector<bool>(data.getNItems(), false));

    for (int i = 0; i < data.getNItems(); i++)
    {
        lambdaItens[i][i] = true;
    }

    master.setOut(env1.getNullStream());

    try
    {
        master.solve();
    }
    catch (IloException &e)
    {
        std::cout << e << '\n';
    }
}

std::pair<int, int> Problema::solve(Node &node)
{
    //Construçao do subproblema
    IloEnv env2;
    IloModel pricingModel(env2);
    IloExpr somaMochila(env2);
    IloBoolVarArray x(env2, data.getNItems());

    for (int i = 0; i < data.getNItems(); i++)
    {
        somaMochila += data.getItemWeight(i) * x[i];
    }

    pricingModel.add(somaMochila <= data.getBinCapacity());
    IloObjective pricingObj(env2);
    pricingModel.add(pricingObj);

    IloCplex pricing(pricingModel);
    pricing.setOut(env2.getNullStream());

    if (!node.is_root)
    {
        std::cout << "juntos: \n";
        //Restriçoes dos itens juntos
        for (auto &p : node.juntos)
        {
            // x_i = x_j = 1
            x[p.first].setLB(1.0);
            x[p.second].setLB(1.0);

            // se lambda não contém o par, eliminá-lo
            for (int i = data.getNItems(); i < lambdaItens.size(); i++)
            {
                //nenhum dos itens está no padrão, ignorar
                if (lambdaItens[i][p.first] == false && lambdaItens[i][p.second] == false)
                {
                    continue;
                }
                //os dois itens estão no padrão, ignorar
                if (lambdaItens[i][p.first] == true && lambdaItens[i][p.second] == true)
                {
                    continue;
                }
                //apenas um dos itens está no padrão
                lambda[i].setUB(0.0);
                std::cout << i << " " << p.first << ", " << p.second << "\n";
            }
        }

        std::cout << "separados: \n";
        //Restriçoes dos itens separados
        for (auto &p : node.separados)
        {
            // adicionar restriçao x_i + x_j <= 1
            pricingModel.add(x[p.first] + x[p.second] <= 1);

            // se lambda contém o par, eliminá-lo
            for (int i = data.getNItems(); i < lambdaItens.size(); i++)
            {
                if (lambdaItens[i][p.first] == true && lambdaItens[i][p.second] == true)
                {
                    lambda[i].setUB(0.0);
                    std::cout << i << " " << p.first << ", " << p.second << "\n";
                }
            }
        }


    }

    z = std::vector<std::vector<double>>(data.getNItems(), std::vector<double>(data.getNItems(), 0));

    //Resolver o mestre pela primeira vez
    try
    {
        master.solve();
    }
    catch (IloException &e)
    {
        std::cout << e << '\n';
    }

    while (1)
    {
        master.getDuals(pi, masterRanges);
        IloExpr somaPricing(env2);

        //FO do pricing a partir das variaveis duais

        for (int i = 0; i < data.getNItems(); i++)
        {
            somaPricing += -pi[i] * x[i];
        }

        pricingObj.setExpr(somaPricing);

        try
        {
            pricing.solve();
        }
        catch (IloException &e)
        {
            std::cout << e << '\n';
        }

        // Primeiro caso de pricing inviavel
        if (pricing.getStatus() == IloAlgorithm::Infeasible)
        {
            prune();
            pricingModel.end();
            env2.end();

            return {0, 0};
        }

        IloNumArray x_values(env2, data.getNItems());

        // Verificar se o custo reduzido é negativo
        if (1 + pricing.getObjValue() < -EPSILON)
        {
            // Verificar antes presença de variaveis artificiais na soluçao
            IloNumArray lambda_values(env1, lambda.getSize());
            master.getValues(lambda_values, lambda);

            std::cout << master.getObjValue() << "\n";

            if (!node.is_root)
            {
                for (int i = 0; i < data.getNItems(); i++)
                {
                    if (lambda_values[i] > EPSILON)
                    {
                        prune();

                        pricingModel.end();
                        env2.end();
                        return {0, 0};
                    }
                }
            }

            // Adicionar nova coluna

            pricing.getValues(x_values, x);
            lambda.add(IloNumVar(masterObj(1) + masterRanges(x_values), 0.0, IloInfinity));

            // Atualizar estrutura auxiliar

            std::vector<bool> itens(data.getNItems(), false);
            for (int i = 0; i < x_values.getSize(); i++)
            {
                if (x_values[i] > 1 - EPSILON)
                {
                    itens[i] = true;
                }
            }

            lambdaItens.push_back(itens);

            try
            {
                master.solve();
            }
            catch (IloException &e)
            {
                std::cout << e << "\n";
            }

            lambda_values.end();
        }
        else
        {
            break;
        }
        x_values.end();
    }

    for (int i = 0; i < lambda.getSize(); i++)
    {
        std::cout << master.getValue(lambda[i]) << "\n";
    }

    // Regra de branching
    IloNumArray lambda_values(env1, lambda.getSize());
    master.getValues(lambda_values, lambda);
    double deltaFrac = std::numeric_limits<double>::infinity();
    double tempDeltaFrac;

    std::pair<int, int> branchingPair;
    for (int i = 0; i < data.getNItems(); i++)
    {
        for (int j = i + 1; j < data.getNItems(); j++)
        {
            for (int k = data.getNItems(); k < lambdaItens.size(); k++)
            {
                //Soma lambda_k a z_ij se o par {i,j} está no padrão
                if (lambdaItens[k][i] == true && lambdaItens[k][j] == true)
                {
                    z[i][j] += lambda_values[k];
                    z[j][i] += lambda_values[k];
                }
            }

            tempDeltaFrac = std::abs(0.5 - z[i][j]);
            if (tempDeltaFrac < deltaFrac)
            {
                branchingPair = {i, j};
                deltaFrac = tempDeltaFrac;
            }
        }
    }

    // Verifica se a soluçao obtida é inteira

    if (std::abs(0.5 - deltaFrac) < EPSILON)
    {
        if (master.getObjValue() < bestInteger)
        {
            bestInteger = master.getObjValue();
        }

        prune();

        pricingModel.end();
        env2.end();
        return {0, 0};
    }

    if (node.is_root)
    {
        master.exportModel("modelo.lp");
    }

    return branchingPair;
}

void Problema::prune()
{
    for (int i = 0; i < lambda.getSize(); i++)
    {
        lambda[i].setUB(IloInfinity);
    }
    std::cout << "PODAR\n";
}