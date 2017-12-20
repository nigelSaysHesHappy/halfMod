#include "halfmod.h"
#include "str_tok.h"
using namespace std;

#define VERSION "v0.1.0"

class voteInfo
{
    public:
        voteInfo();
        void set(string agenda, string candis[], int size);
        ~voteInfo();
        void reset();
        string ballad;
        string *candidates;
        string *voters;
        int *votes;
        int quantity;
        int voteCount;
        //int (*outcome)(string,int[],int);
};

voteInfo vote;

extern "C" {

void onPluginStart(hmHandle &handle, hmGlobal *global)
{
    recallGlobal(global);
    handle.pluginInfo("Votes",
                      "nigel",
                      "Vote system",
                      VERSION,
                      "http://the.ballad.justca.me/in/your/box");
    handle.regConsoleCmd("hm_vote","voteCmd","Cast a vote");
    handle.regAdminCmd("hm_createvote","createVoteCmd",FLAG_VOTE,"Initiate a vote");
}

int onPlayerDisconnect(hmHandle &handle, smatch args)
{
    if (vote.quantity > 1)
    {
        string name = args[1].str();
        string client = stripFormat(lower(name));
        hmGlobal *global;
        global = recallGlobal(global);
        int can = -1;
        for (auto it = global->players.begin(), ite = global->players.end();it != ite;++it)
        {
            if (stripFormat(lower(it->name)) == client)
            {
                string lines = it->custom;
                for (string line;(line = gettok(lines,1,"\n")) != "";lines = deltok(lines,1,"\n"))
                {
                    if (gettok(line,1,"=") == "vote")
                    {
                        can = stoi(gettok(line,2,"="));
                        vote.votes[can]--;
                        bool found = false;
                        for (int i = 0;i < vote.voteCount;i++)
                        {
                            if (vote.voters[i] == client)
                                found = true;
                            if (found)
                            {
                                if (i+1 != vote.voteCount)
                                    vote.voters[i] = vote.voters[i+1];
                                else
                                {
                                    vote.voters[i].clear();
                                    break;
                                }
                            }
                        }
                        vote.voteCount--;
                        hmSendMessageAll(name + "'s vote has been thrown out!");
                        break;
                    }
                }
                break;
            }
        }
    }
    return 0;
}

int voteCmd(hmHandle &handle, string client, string args[], int argc)
{
    if (client == "#SERVER")
    {
        hmReplyToClient(client,"Only online players are allowed to vote!");
        return 1;
    }
    if (vote.quantity == 0)
    {
        hmReplyToClient(client,"There is no vote currently in progress.");
        return 1;
    }
    if (argc < 2)
    {
        hmReplyToClient(client," " + vote.ballad + "?");
        for (int i = 0;i < vote.quantity;i++)
            hmReplyToClient(client,"  " + to_string(i+1) + ". " + vote.candidates[i]);
        hmReplyToClient(client,"Usage: " + args[0] + " <#>");
        return 1;
    }
    // check candidate exists
    int can = stoi(args[1])-1;
    if ((can < 0) || (can >= vote.quantity))
    {
        hmReplyToClient(client,"Invalid candidate (" + args[1] + ")");
        return 1;
    }
    client = stripFormat(lower(client));
    for (int i = 0;i < vote.voteCount;i++)
    {
        if (vote.voters[i] == client)
        {
            hmReplyToClient(client,"You have already voted!");
            return 1;
        }
    }
    hmGlobal *global;
    global = recallGlobal(global);
    for (auto it = global->players.begin(), ite = global->players.end();it != ite;++it)
    {
        if (stripFormat(lower(it->name)) == client)
        {
            it->custom = addtok(it->custom,"vote=" + to_string(can),"\n");
            break;
        }
    }
    vote.votes[can]++;
    vote.voters[vote.voteCount++] = client;
    //vote.voteCount++;
    if (vote.voteCount >= global->players.size())
        handle.triggerTimer("voteTimer");
    else for (auto it = handle.timers.begin(), ite = handle.timers.end();it != ite;++it)
        if (it->name == "voteTimer")
            it->interval += 60;
    hmReplyToClient(client,"Your vote has been cast. Thanks for voting!");
    return 0;
}

int createVoteCmd(hmHandle &handle, string client, string args[], int argc)
{
    if (argc < 2)
    {
        hmReplyToClient(client,"Usage: " + args[0] + " <agenda>");
        hmReplyToClient(client,"Usage: " + args[0] + " <agenda> <candidate 1> <candidate 2> [more candidates ...]");
        return 1;
    }
    if (argc == 3)
    {
        hmReplyToClient(client,"Usage: " + args[0] + " <agenda>");
        hmReplyToClient(client,"Usage: " + args[0] + " <agenda> <candidate 1> <candidate 2> [more candidates ...]");
        return 1;
    }
    if (vote.quantity > 1)
    {
        hmReplyToClient(client,"There is already a vote in progress.");
        return 1;
    }
    if (argc < 3)
    {
        string yesno[] = {"Yes","No"};
        vote.set(args[1],yesno,2);
    }
    else
        vote.set(args[1],args+2,argc-2);
    handle.createTimer("voteTimer",90,"tallyFunc");
    hmSendMessageAll("The polls have opened! Type '!vote #' to cast your vote");
    hmSendMessageAll(" " + vote.ballad + "?");
    for (int i = 0;i < vote.quantity;i++)
        hmSendMessageAll("  " + to_string(i+1) + ". " + vote.candidates[i]);
    return 0;
}

int tallyFunc(hmHandle &handle, string args)
{
    hmGlobal *global;
    global = recallGlobal(global);
    //hmSendMessageAll("The polls have closed! Tallying up the votes.");
    for (auto it = global->players.begin(), ite = global->players.end();it != ite;++it)
    {
        for (int i = 1, j = numtok(it->custom,"\n");i <= j;i++)
        {
            if (gettok(gettok(it->custom,i,"\n"),1,"=") == "vote")
            {
                it->custom = deltok(it->custom,i,"\n");
                break;
            }
        }
    }
    if (vote.voteCount < 1)
        hmSendMessageAll("The polls have closed! Not one vote was cast!");
    else
    {
        int temp = 0, i;
        vector<int> winners;
        for (i = 0;i < vote.quantity;i++)
            if (vote.votes[i] > temp)
                temp = vote.votes[i];
        for (i = 0;i < vote.quantity;i++)
            if (vote.votes[i] == temp)
                winners.push_back(i);
        float percent = 100.0 / float(vote.voteCount) * float(temp);
        string msg = "'" + vote.candidates[winners[0]] + "'";
        if (winners.size() == 1)
            hmSendMessageAll(data2str("The polls have closed! %s wins with %.2f%% of the votes!",msg.c_str(),percent));
        else
        {
            if (winners.size() == 2)
                msg = msg + " and '" + vote.candidates[winners[1]] + "'";
            else for (auto it = winners.begin()+1, ite = winners.end();it != ite;++it)
            {
                if (it+1 == ite)
                    msg = msg + ", and '" + vote.candidates[*it] + "'";
                else
                    msg = msg + ", " + vote.candidates[*it] + "'";
            }
            hmSendMessageAll(data2str("The polls have closed! %s have tied with %.2f%% of the votes!",msg.c_str(),percent));
        }
    }
    vote.reset();
    return 1;
}

}

voteInfo::voteInfo()
{
    voteCount = 0;
    quantity = 0;
}

void voteInfo::set(string agenda, string candis[], int size)
{
    ballad = agenda;
    candidates = new string[size];
    for (int i = 0;i < size;i++)
        candidates[i] = candis[i];
    int max = recallGlobal(NULL)->maxPlayers;
    voters = new string[max];
    for (int i = 0;i < max;i++)
        voters[i].clear();
    votes = new int[size];
    for (int i = 0;i < size;i++)
        votes[i] = 0;
    quantity = size;
    voteCount = 0;
}

void voteInfo::reset()
{
    ballad.clear();
    quantity = 0;
    voteCount = 0;
    delete[] candidates;
    delete[] voters;
    delete[] votes;
}

voteInfo::~voteInfo()
{
    reset();
}

