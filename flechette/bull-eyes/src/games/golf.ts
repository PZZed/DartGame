// src/games/GameGolf.ts
import type { IGameLogic, Player } from '@/stores/game'

export class GameGolf implements IGameLogic {
  private players: Player[] = []
  private currentPlayerIndex = 0
  start (playerNames: string[]): void {
    this.players = playerNames.map(name => ({ name, score: 0, darts: [] }))
  }

  throw (input: string): void {}

  getScore (): number {
    return 0
  }

  nextPlayer (): number {
    return 0
  }

  isWon (): boolean {
    return true
  }

  getPlayers (): Player [] {
    return this.players
  }

  gameName (): string {
    return 'golf'
  }
}
