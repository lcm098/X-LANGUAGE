

struct {
    var weapon_name;
    var damage;
}Weapon;

struct {
    struct Weapon weapon;
    let player_name;
    let player_id;
    let p_profile;
}Player;


struct Player danishk;

danishk.player_name = "danishk";
danishk.player_id = 2332;
danishk.p_profile = "thisisdanishk@gmail.com";
danishk.weapon.weapon_name = "ak47";
danishk.weapon.damage = 40;

print danishk.player_name;
print danishk.player_id;
print danishk.weapon.weapon_name;
print danishk.weapon.damage;