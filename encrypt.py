import json
from Crypto.Cipher import AES
from Crypto.Util.Padding import pad

KEY = bytes.fromhex("b6b004752453d0a85b509ea0605d19a45f496f09bf701d5ae8fabc60a4772b6c")
IV  = bytes.fromhex("00010203040506070809101112131415")

def aes_encrypt(data: bytes) -> bytes:
    cipher = AES.new(KEY, AES.MODE_CBC, IV)
    return cipher.encrypt(pad(data, AES.block_size))

ENTRIES = [
    ("https://google.com",       "alice",      "qwerty123"),
    ("https://yandex.ru",        "bob",        "password1"),
    ("https://github.com",       "charlie",    "gh_token_99"),
    ("https://mail.ru",          "diana",      "mailPass!1"),
    ("https://vk.com",           "eve",        "vkontakte22"),
    ("https://youtube.com",      "frank",      "ytube#pass"),
    ("https://reddit.com",       "grace",      "r3dd1t_pw"),
    ("https://twitter.com",      "heidi",      "tw1tt3r!"),
    ("https://facebook.com",     "ivan",       "fb_secure9"),
    ("https://instagram.com",    "judy",       "insta2024!"),
    ("https://linkedin.com",     "karl",       "link_prof1"),
    ("https://stackoverflow.com","laura",      "st4ck0ver"),
    ("https://amazon.com",       "mike",       "amzn_shop1"),
    ("https://netflix.com",      "nina",       "n3tfl1x!!"),
    ("https://spotify.com",      "oscar",      "sp0t1fy_pw"),
    ("https://twitch.tv",        "peggy",      "tw1tch_gg"),
    ("https://discord.com",      "quinn",      "d1sc0rd#1"),
    ("https://telegram.org",     "rupert",     "tg_s3cur3"),
    ("https://whatsapp.com",     "sarah",      "wh4ts4pp!"),
    ("https://zoom.us",          "trent",      "z00m_mtg1"),
    ("https://dropbox.com",      "ursula",     "dr0pb0x22"),
    ("https://notion.so",        "victor",     "n0t10n_pw"),
    ("https://figma.com",        "wendy",      "f1gm4_des"),
    ("https://slack.com",        "xander",     "sl4ck_wrk"),
    ("https://trello.com",       "yvonne",     "tr3ll0_bd"),
    ("https://gitlab.com",       "zach",       "g1tl4b_ci"),
    ("https://bitbucket.org",    "anna",       "b1tbck_rp"),
    ("https://medium.com",       "boris",      "m3d1um_wr"),
    ("https://wikipedia.org",    "clara",      "w1k1_read"),
    ("https://apple.com",        "dmitry",     "4ppl3_id!"),
    ("https://microsoft.com",    "elena",      "msft_az1!"),
    ("https://steam.com",        "felix",      "st34m_gm1"),
    ("https://epicgames.com",    "greta",      "3p1c_frtn"),
    ("https://paypal.com",       "henry",      "p4yp4l_$$"),
    ("https://ebay.com",         "irene",      "3b4y_b1d!"),
    ("https://aliexpress.com",   "james",      "4l1_sh0p1"),
    ("https://booking.com",      "kate",       "b00k_htl!"),
    ("https://airbnb.com",       "leo",        "41rbnb_go"),
    ("https://uber.com",         "maria",      "ub3r_r1de"),
    ("https://pinterest.com",    "nick",       "p1n_b0ard"),
]

def generate_encrypted_vault(output_path: str):
    creds = []
    for url, login, password in ENTRIES:
        # 1-й слой: шифруем пару логин/пароль -> hex
        secret_json = json.dumps({"login": login, "password": password}, separators=(',', ':')).encode("utf-8")
        secret_cipher = aes_encrypt(secret_json)
        secret_hex = secret_cipher.hex()

        creds.append({"url": url, "secret": secret_hex})

    # Собираем итоговый JSON
    vault = {"creds": creds}
    vault_json = json.dumps(vault, indent=4, ensure_ascii=False).encode("utf-8")

    print(f"JSON перед 2-м слоем ({len(vault_json)} байт):")
    print(vault_json[:200].decode("utf-8"), "...")
    print()

    # 2-й слой: шифруем весь JSON файл
    vault_cipher = aes_encrypt(vault_json)

    with open(output_path, "wb") as f:
        f.write(vault_cipher)

    print(f"Записано: {output_path}")
    print(f"Записей: {len(creds)}")
    print(f"Размер: {len(vault_json)} -> {len(vault_cipher)} байт")

if __name__ == "__main__":
    output = "password-manager-Zamotina-221-331/build/Desktop_Qt_6_10_2_MinGW_64_bit-Debug/credentials.json"
    generate_encrypted_vault(output)
    print("Готово!")