<template>
  <div class="main-layout">
    <!-- Top Navigation Bar -->
    <header class="topbar">
      <div class="topbar-inner">
        <!-- Brand -->
        <router-link to="/dashboard" class="brand">
          <div class="brand-mark">
            <svg width="20" height="20" viewBox="0 0 32 32" fill="none">
              <path d="M16 2L2 16L16 30L30 16L16 2Z" stroke="currentColor" stroke-width="1.5" fill="none"/>
              <path d="M16 8L8 16L16 24L24 16L16 8Z" fill="currentColor" opacity="0.35"/>
            </svg>
          </div>
          <span class="brand-name">OxyRent</span>
        </router-link>

        <!-- Nav -->
        <nav class="topbar-nav">
          <router-link
            v-for="item in allMenuItems"
            :key="item.path"
            :to="item.path"
            class="nav-pill"
            :class="{ active: activeMenu === item.path }"
          >
            <el-icon :size="16"><component :is="item.icon" /></el-icon>
            <span>{{ item.label }}</span>
          </router-link>
        </nav>

        <!-- User -->
        <div class="topbar-right">
          <el-dropdown trigger="click" @command="handleUserCommand">
            <div class="user-pill">
              <div class="user-avatar">{{ (authStore.username || 'G')[0].toUpperCase() }}</div>
              <span class="username">{{ authStore.username || 'Guest' }}</span>
              <el-icon class="arrow"><ArrowDown /></el-icon>
            </div>
            <template #dropdown>
              <el-dropdown-menu>
                <el-dropdown-item command="profile">
                  <el-icon><User /></el-icon> 个人信息
                </el-dropdown-item>
                <el-dropdown-item command="logout" divided>
                  <el-icon><SwitchButton /></el-icon> 退出登录
                </el-dropdown-item>
              </el-dropdown-menu>
            </template>
          </el-dropdown>
        </div>
      </div>

    </header>

    <!-- Page Content -->
    <main class="content-area">
      <router-view v-slot="{ Component, route }">
        <transition name="page-fade" mode="out-in">
          <component :is="Component" :key="route.path" />
        </transition>
      </router-view>
    </main>

    <!-- Bottom Dock -->
    <div class="dock">
      <div class="dock-inner">
        <button
          v-for="action in dockActions"
          :key="action.path"
          class="dock-btn"
          @click="handleDockAction(action)"
          :title="action.label"
        >
          <el-icon :size="18"><component :is="action.icon" /></el-icon>
          <span class="dock-label">{{ action.label }}</span>
        </button>
      </div>
    </div>
  </div>
</template>

<script setup>
import { computed, watch } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { useAuthStore } from '@/stores/auth'
import { useAppStore } from '@/stores/app'
import {
  HomeFilled, Van, Document, SetUp, Wallet, Tickets, DataLine,
  User, ArrowDown, SwitchButton, Plus,
} from '@element-plus/icons-vue'

const route = useRoute()
const router = useRouter()
const authStore = useAuthStore()
const appStore = useAppStore()

const mainMenu = [
  { path: '/dashboard', label: '工作台', icon: 'HomeFilled' },
  { path: '/vehicles', label: '车辆', icon: 'Van' },
  { path: '/rentals', label: '租赁', icon: 'Document' },
]

const staffMenu = [
  { path: '/maintenance', label: '维保', icon: 'SetUp' },
  { path: '/topup', label: '充值', icon: 'Wallet' },
]

const adminMenu = [
  { path: '/payments', label: '收费', icon: 'Wallet' },
  { path: '/invoices', label: '账单', icon: 'Tickets' },
  { path: '/statistics', label: '报表', icon: 'DataLine' },
  { path: '/users', label: '用户', icon: 'User' },
]

const allMenuItems = computed(() => {
  const items = [...mainMenu]
  if (authStore.isAdmin) items.push(...adminMenu)
  else if (authStore.isStaff) items.push(...staffMenu)
  return items
})

const dockActions = computed(() => {
  const actions = [
    { path: '/rentals/create', label: '新租赁', icon: 'Plus' },
  ]
  if (authStore.isAdmin) {
    actions.push({ path: '/vehicles/add', label: '添加车辆', icon: 'Van' })
  }
  actions.push(
    { path: '/balance', label: '余额', icon: 'Wallet' },
    { path: '/profile', label: '我的', icon: 'User' },
  )
  return actions
})

const activeMenu = computed(() => {
  const path = route.path
  if (path.startsWith('/vehicles')) return '/vehicles'
  if (path.startsWith('/rentals')) return '/rentals'
  if (path.startsWith('/maintenance')) return '/maintenance'
  if (path.startsWith('/payments')) return '/payments'
  if (path.startsWith('/invoices')) return '/invoices'
  if (path.startsWith('/statistics')) return '/statistics'
  if (path.startsWith('/users')) return '/users'
  return path
})

const pageTitle = computed(() => route.meta?.title || '')

watch(pageTitle, (title) => {
  appStore.setPageTitle(title)
}, { immediate: true })

function handleUserCommand(cmd) {
  if (cmd === 'profile') router.push('/profile')
  else if (cmd === 'logout') authStore.logout()
}

function handleDockAction(action) {
  router.push(action.path)
}
</script>

<style scoped>
.main-layout {
  min-height: 100vh;
  background: var(--bg-primary);
  display: flex;
  flex-direction: column;
}

/* ===== Top Bar ===== */
.topbar {
  position: sticky;
  top: 0;
  z-index: 100;
  background: var(--bg-secondary);
  border-bottom: 1px solid var(--border);
}

.topbar-inner {
  height: 56px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 24px;
  gap: 24px;
}

/* Brand */
.brand {
  display: flex;
  align-items: center;
  gap: 10px;
  text-decoration: none;
  flex-shrink: 0;
}

.brand-mark {
  width: 32px;
  height: 32px;
  display: flex;
  align-items: center;
  justify-content: center;
  color: var(--accent);
  border: 1px solid var(--border-accent);
  border-radius: 8px;
}

.brand-name {
  font-family: var(--font-display);
  font-size: 18px;
  font-weight: 500;
  color: var(--text-primary);
  letter-spacing: 1px;
}

/* Primary Nav */
.topbar-nav {
  display: flex;
  align-items: center;
  gap: 4px;
}

.nav-pill {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 7px 14px;
  border-radius: 8px;
  font-size: 13px;
  font-weight: 500;
  color: var(--text-secondary);
  text-decoration: none;
  transition: all 0.2s var(--ease);
  white-space: nowrap;
}

.nav-pill:hover {
  background: rgba(255, 255, 255, 0.04);
  color: var(--text-primary);
}

.nav-pill.active {
  background: var(--accent-muted);
  color: var(--accent);
}

/* User */
.topbar-right {
  flex-shrink: 0;
}

.user-pill {
  display: flex;
  align-items: center;
  gap: 8px;
  cursor: pointer;
  padding: 5px 12px 5px 5px;
  border-radius: 100px;
  border: 1px solid var(--border);
  background: transparent;
  transition: all 0.2s var(--ease);
}

.user-pill:hover {
  border-color: var(--border-hover);
  background: rgba(255, 255, 255, 0.02);
}

.user-avatar {
  width: 26px;
  height: 26px;
  border-radius: 50%;
  background: var(--accent-muted);
  color: var(--accent);
  display: flex;
  align-items: center;
  justify-content: center;
  font-size: 11px;
  font-weight: 600;
  font-family: var(--font-body);
}

.username {
  font-size: 13px;
  color: var(--text-primary);
  font-weight: 400;
}

.arrow {
  font-size: 10px;
  color: var(--text-tertiary);
}

/* ===== Content ===== */
.content-area {
  flex: 1;
  padding: 0 24px 100px;
  width: 100%;
  max-width: 1400px;
  margin: 0 auto;
}

/* ===== Page Transition ===== */
.page-fade-enter-active {
  transition: opacity 0.25s var(--ease-out);
}
.page-fade-leave-active {
  transition: opacity 0.15s var(--ease);
}
.page-fade-enter-from,
.page-fade-leave-to {
  opacity: 0;
}

/* ===== Bottom Dock ===== */
.dock {
  position: fixed;
  bottom: 20px;
  left: 50%;
  transform: translateX(-50%);
  z-index: 100;
}

.dock-inner {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 6px 8px;
  background: var(--bg-elevated);
  border: 1px solid var(--border);
  border-radius: 16px;
  box-shadow: var(--shadow-lg);
}

.dock-btn {
  display: flex;
  align-items: center;
  gap: 6px;
  padding: 8px 12px;
  border-radius: 10px;
  border: none;
  background: transparent;
  color: var(--text-secondary);
  cursor: pointer;
  transition: all 0.2s var(--ease);
  font-family: var(--font-body);
  font-size: 12px;
  font-weight: 500;
  white-space: nowrap;
}

.dock-btn:hover {
  background: rgba(255, 255, 255, 0.06);
  color: var(--text-primary);
}

.dock-btn:active {
  transform: scale(0.96);
}

.dock-label {
  max-width: 0;
  overflow: hidden;
  opacity: 0;
  transition: max-width 0.25s var(--ease), opacity 0.2s var(--ease);
}

.dock-btn:hover .dock-label {
  max-width: 80px;
  opacity: 1;
}

/* ===== Responsive ===== */
@media (max-width: 768px) {
  .topbar-inner {
    padding: 0 16px;
    gap: 12px;
  }

  .brand-name {
    display: none;
  }

  .nav-pill span {
    display: none;
  }

  .nav-pill {
    padding: 7px 10px;
  }

  .username {
    display: none;
  }

  .content-area {
    padding: 0 16px 100px;
  }

  .dock {
    bottom: 12px;
  }

  .dock-label {
    display: none;
  }

  .dock-btn {
    padding: 10px;
  }
}

@media (prefers-reduced-motion: reduce) {
  .page-fade-enter-active,
  .page-fade-leave-active {
    transition: none;
  }
  .nav-pill,
  .sub-pill,
  .dock-btn,
  .dock-label {
    transition: none;
  }
}
</style>
