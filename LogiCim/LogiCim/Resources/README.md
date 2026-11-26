# Resources

What LogiCim loads at run time from its folder's `Resources` (`LogicimApp.cpp`): its fonts, its icons and the logic
families' timings.

| Files | Are | From |
|---|---|---|
| `InterVariable.ttf` | Third-party | Inter 4.001 (variable), by Rasmus Andersson, rsms.me/inter, under the SIL Open Font License 1.1. |
| `Fonts/*.ttf`, `Fonts/LiberationFonts-LICENSE.txt` | Third-party | Liberation Mono and Sans 2.1.5, by Red Hat, github.com/liberationfonts, under the SIL Open Font License 1.1 (the licence beside them). |
| `Icons/*.png` | Generated | `python3 Tools/ConvertLogicimIcons.py <Logicim>/res/icons LogiCim/Resources/Icons`, run in `LogiCim/LogiCim`: Logicim's KiCad style icons, recoloured for IntelliJ IDEA's dark theme. |
| `PropagationTimes.csv` | Source | Each logic family's timings and levels, from its maker's datasheet (the Manufacturer column).  LVC's are the 74LVC541's, which runs from 1.2V to 3.6V; its 5V rows are the single gates', which run to 5.5V (the 74LVC1G04's delays and levels, the 74LVC1G125's enable and disable times), as only they are on the HCP65's 5V rails. |
