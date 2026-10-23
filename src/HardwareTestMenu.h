#pragma once
// Include after the TextMenu definition; shared with the main menu factory.
class HardwareTestMenu : public TextMenu {
public:
    HardwareTestMenu();
    ~HardwareTestMenu() override;
    int onSelect() override;
    void drawList(SDL_Surface*) override;
    void freeListItem() override;
    void initializeTests();
    int stressSeconds;
};
