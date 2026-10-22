#pragma once
// Include after the TextMenu definition; shared with the main menu factory.
class HardwareTestMenu : public TextMenu {
public:
    HardwareTestMenu();
    ~HardwareTestMenu() override;
    int vfunc4() override;
    void vfunc7(SDL_Surface*) override;
    void vfunc18() override;
    void initializeTests();
    int stressSeconds;
};
