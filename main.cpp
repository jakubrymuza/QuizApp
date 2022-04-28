#include <gpiod.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>

#define QUESTIONS_COUNT 5
#define CONSUMER "Consumer"

using namespace std;

class Question
{
    static const int AnswersCount = 3;
    string Content;
    string *Answers;
    int CorrectAnswer;

public:
    Question(string content, string answers[], int correctAnswer);

    ~Question();

    void Print();

    bool IsCorrect(int choosenAnswer);

    void PrintAnswer(int choosenAnswer);
};

bool Setup();
bool SetupButton(unsigned int line_num, struct gpiod_line **line);
bool SetupLED(unsigned int line_num, struct gpiod_line **line);
void Cleanup();

int CheckButton(struct gpiod_line **line);
Question **PrepareQuestions();
void SignalAnswer(bool correct);
int GetInput();
int AskQuestion(Question *question);

struct gpiod_chip *chip;
struct gpiod_line *line_led_good;
struct gpiod_line *line_led_bad;

struct gpiod_line *line_button_0;
struct gpiod_line *line_button_1;
struct gpiod_line *line_button_2;
struct gpiod_line *line_button_3;

int main(int argc, char **argv)
{
    if (!Setup())
        return 0;

    cout << "Program Quiz" << endl;
    cout << "Autor: Jakub Rymuza" << endl;

    Question **questions = PrepareQuestions();

    int correct = 0;
    int asked = 0;

    for (int i = 0; i < QUESTIONS_COUNT; ++i)
    {
        ++asked;
        int ret = AskQuestion(questions[i]);
        
        if(ret == -1)
            break;

        correct += ret;
    }

    if (asked == QUESTIONS_COUNT)
        cout << "Odpowiedziales na wszystkie pytania." << endl;

    cout << "Wynik: " << correct << "/" << asked << endl;


    for (int i = 0; i < QUESTIONS_COUNT; ++i)
        delete questions[i];

    delete[] questions;

    Cleanup();

    return 0;
}

// zadaj pytanie
int AskQuestion(Question *question)
{
    question->Print();

    int answer = GetInput();

    // zakończ quiz
    if (answer == 0)
        return -1;

    question->PrintAnswer(answer);

    SignalAnswer(question->IsCorrect(answer));

    return question->IsCorrect(answer);
}

// oczekuj na wcisniecie przycisku
int GetInput()
{
    int ret;

    while (true)
    {
        // konczenie programu
        ret = gpiod_line_get_value(line_button_0);
        if (ret < 0)
            return 0;
        if (ret == 0)
            return 0;

        // odpowiedzi a,b,c
        ret = gpiod_line_get_value(line_button_1);
        if (ret < 0)
            return 0;
        if (ret == 0)
            return 1;

        ret = gpiod_line_get_value(line_button_2);
        if (ret < 0)
            return 0;
        if (ret == 0)
            return 2;

        ret = gpiod_line_get_value(line_button_3);
        if (ret < 0)
            return 0;
        if (ret == 0)
            return 3;
    }

    return 0;
}

// sprawdza czy przycisk wcisniety
int CheckButton(struct gpiod_line **line)
{
    int ret = gpiod_line_get_value(*line);
    if (ret < 0)
    {
        perror("Request line as output failed\n");
        gpiod_line_release(*line);
        return -1;
    }

    return ret;
}


// zapala odpowiedni led w zaleznosci od poprawnosci odpowiedzi
void SignalAnswer(bool correct)
{
    struct gpiod_line *line;

    if (correct)
        line = line_led_good;
    else
        line = line_led_bad;

    if (gpiod_line_set_value(line, 1) < 0)
    {
        perror("Set line output failed\n");
        gpiod_line_release(line);
    }
    sleep(1);
    if (gpiod_line_set_value(line, 0) < 0)
    {
        perror("Set line output failed\n");
        gpiod_line_release(line);
    }
}

// setup przyciskow i LEDow
bool Setup()
{
    char chipname[] = "gpiochip0";
    unsigned int line_num_led_good = 23;
    unsigned int line_num_led_bad = 27;
    unsigned int line_num_button_0 = 18;
    unsigned int line_num_button_1 = 17;
    unsigned int line_num_button_2 = 10;
    unsigned int line_num_button_3 = 25;

    chip = gpiod_chip_open_by_name(chipname);
    if (!chip)
    {
        perror("Open chip failed\n");
        return false;
    }

    if (!SetupLED(line_num_led_good, &line_led_good))
        return false;
    if (!SetupLED(line_num_led_bad, &line_led_bad))
        return false;
    if (!SetupButton(line_num_button_0, &line_button_0))
        return false;
    if (!SetupButton(line_num_button_1, &line_button_1))
        return false;
    if (!SetupButton(line_num_button_2, &line_button_2))
        return false;
    if (!SetupButton(line_num_button_3, &line_button_3))
        return false;

    return true;
}

// setup ledu
bool SetupLED(unsigned int line_num, struct gpiod_line **line)
{
    *line = gpiod_chip_get_line(chip, line_num);
    if (!line)
    {
        perror("Get line failed\n");
        gpiod_chip_close(chip);
        return false;
    }

    if (gpiod_line_request_output(*line, CONSUMER, 0) < 0)
    {
        perror("Request line as output failed\n");
        gpiod_line_release(*line);
        return false;
    }

    return true;
}

// setup przycisku
bool SetupButton(unsigned int line_num, struct gpiod_line **line)
{
    *line = gpiod_chip_get_line(chip, line_num);
    if (!line)
    {
        perror("Get line failed\n");
        gpiod_chip_close(chip);
        return false;
    }

    if (gpiod_line_request_input(*line, CONSUMER) < 0)
    {
        perror("Request line as input failed\n");
        gpiod_line_release(*line);
        return false;
    }

    return true;
}

// czyszczenie
void Cleanup()
{
    gpiod_line_release(line_led_good);
    gpiod_line_release(line_led_bad);

    gpiod_line_release(line_button_0);
    gpiod_line_release(line_button_1);
    gpiod_line_release(line_button_2);
    gpiod_line_release(line_button_3);

    gpiod_chip_close(chip);
}

// pytania
Question **PrepareQuestions()
{
    string answers1[] = {"1410r.", "1353r.", "1420r."};
    Question *question1 = new Question("Podaj date bitwy pod Grunwaldem.", answers1, 1);

    string answers2[] = {"6 dni", "7 dni", "8 dni"};
    Question *question2 = new Question("Ile jest dni tygodnia?", answers2, 2);

    string answers3[] = {"8.00 p.m.", "9.00 a.m.", "9.00 p.m."};
    Question *question3 = new Question("Godzina 21.00 to jaka godzina w formacie 12 godzinnym?", answers3, 3);

    string answers4[] = {"1", "3", "6"};
    Question *question4 = new Question("Ile zon mial krol Anglii Henryk XIII", answers4, 3);

    string answers5[] = {"29", "30", "31"};
    Question *question5 = new Question("Ile dni ma kwiecien?", answers5, 2);

    Question **questions = new Question *[QUESTIONS_COUNT]
    {
        question1, question2, question3, question4, question5
    };

    return questions;
}

// metody klasy Question

Question::Question(string content, string answers[], int correctAnswer)
{
    Content = content;
    Answers = new string[AnswersCount];
    for (int i = 0; i < AnswersCount; ++i)
        Answers[i] = answers[i];

    CorrectAnswer = correctAnswer;
}

Question::~Question()
{
    delete[] Answers;
}

void Question::Print()
{
    cout << Content << endl;
    for (int i = 0; i < AnswersCount; ++i)
        cout << i + 1 << ") " << Answers[i] << endl;
}

bool Question::IsCorrect(int choosenAnswer)
{
    return choosenAnswer == CorrectAnswer;
}

void Question::PrintAnswer(int choosenAnswer)
{
    cout << "Wybrano odpowiedz " << choosenAnswer << "." << endl;

    if (IsCorrect(choosenAnswer))
        cout << "Prawdilowa odpowiedz!" << endl;
    else
    {
        cout << "Bledna odpowiedz!" << endl;
        cout << "Poprawna odpowiedz to: " << CorrectAnswer << " - " << Answers[CorrectAnswer - 1] << endl;
    }
}
