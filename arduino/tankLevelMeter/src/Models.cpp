#include <Arduino.h>

class FourLineDisplayModel
{
    private:
    String Line1;
    String Line2;
    String Line3;
    String Line4;

    public:
    FourLineDisplayModel(String l1, String l2, String l3, String l4)
    {
        Line1 = "L: " + l1;
        Line2 = l2;
        Line3 = l3;
        Line4 = l4;
    };

    String FormatLine1()
    {
        return Line1 + " cm";
    }
};
