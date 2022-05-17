class FourLineDisplayModel
{
    private:
    string Line1 = "L: ";
    string Line2;
    string Line3;
    string Line4;

    public: 
    FourLineDisplayModel(string l1, string l2, string l3, string l4)
    {
        Line1.concat(l1).concat();
        Line2 = l2;
        Line3 = l3;
        Line4 = l4;
    };

    string FormatLine1()
    {
        return Line1.concat(" cm");
    }
}