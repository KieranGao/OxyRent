<template>
  <div class="main-layout" :class="{ 'sidebar-collapsed': appStore.sidebarCollapsed }">
    <!-- Sidebar -->
    <aside class="sidebar">
      <div class="sidebar-logo">
        <div class="logo-glow"></div>
        <span v-if="!appStore.sidebarCollapsed" class="logo-text">OxyRent</span>
        <span v-else class="logo-icon">OR</span>
      </div>

      <el-menu
        :default-active="activeMenu"
        :collapse="appStore.sidebarCollapsed"
        :collapse-transition="false"
        background-color="var(--bg-sidebar)"
        text-color="var(--text-sidebar)"
        active-text-color="var(--text-sidebar-active)"
        router
      >
        <el-menu-item index="/dashboard">
          <el-icon><HomeFilled /></el-icon>
          <span>Dashboard</span>
        </el-menu-item>
        <el-menu-item index="/vehicles">
          <el-icon><Van /></el-icon>
          <span>Vehicles</span>
        </el-menu-item>
        <el-menu-item index="/rentals">
          <el-icon><Document /></el-icon>
          <span>Rentals</span>
        </el-menu-item>
        <el-menu-item index="/maintenance" v-if="authStore.isAdmin">
          <el-icon><SetUp /></el-icon>
          <span>Maintenance</span>
        </el-menu-item>

        <template v-if="authStore.isAdmin">
          <el-divider style="margin: 8px 0; border-color: rgba(255,255,255,0.08)" />
          <div v-if="!appStore.sidebarCollapsed" class="menu-group-title">Admin</div>
          <el-menu-item index="/payments">
            <el-icon><Wallet /></el-icon>
            <span>Payments</span>
          </el-menu-item>
          <el-menu-item index="/invoices">
            <el-icon><Tickets /></el-icon>
            <span>Invoices</span>
          </el-menu-item>
          <el-menu-item index="/statistics">
            <el-icon><DataLine /></el-icon>
            <span>Statistics</span>
          </el-menu-item>
          <el-menu-item index="/users">
            <el-icon><User /></el-icon>
            <span>Users</span>
          </el-menu-item>
        </template>
      </el-menu>
    </aside>

    <!-- Main Content Area -->
    <div class="main-area">
      <!-- Header -->
      <header class="header">
        <div class="header-left">
          <el-button
            class="collapse-btn"
            :icon="appStore.sidebarCollapsed ? Expand : Fold"
            text
            @click="appStore.toggleSidebar()"
          />
          <el-breadcrumb separator="/">
            <el-breadcrumb-item :to="{ path: '/dashboard' }">Home</el-breadcrumb-item>
            <el-breadcrumb-item v-if="pageTitle">{{ pageTitle }}</el-breadcrumb-item>
          </el-breadcrumb>
        </div>

        <div class="header-right">
          <el-switch
            v-model="isDark"
            :active-action-icon="Moon"
            :inactive-action-icon="Sunny"
            inline-prompt
            size="small"
            @change="appStore.toggleTheme()"
          />

          <el-dropdown trigger="click" @command="handleUserCommand">
            <span class="user-info">
              <el-avatar :size="32" icon="UserFilled" />
              <span class="username">{{ authStore.username || 'Guest' }}</span>
              <el-icon class="arrow"><ArrowDown /></el-icon>
            </span>
            <template #dropdown>
              <el-dropdown-menu>
                <el-dropdown-item command="profile">
                  <el-icon><User /></el-icon> Profile
                </el-dropdown-item>
                <el-dropdown-item command="logout" divided>
                  <el-icon><SwitchButton /></el-icon> Logout
                </el-dropdown-item>
              </el-dropdown-menu>
            </template>
          </el-dropdown>
        </div>
      </header>

      <!-- Page Content -->
      <main class="content">
        <router-view v-slot="{ Component, route }">
          <transition name="slide-fade" mode="out-in">
            <component :is="Component" :key="route.path" />
          </transition>
        </router-view>
      </main>
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
  User, Expand, Fold, Moon, Sunny, ArrowDown, SwitchButton,
} from '@element-plus/icons-vue'

const route = useRoute()
const router = useRouter()
const authStore = useAuthStore()
const appStore = useAppStore()

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
const isDark = computed({
  get: () => appStore.theme === 'dark',
  set: () => {},
})

watch(pageTitle, (title) => {
  appStore.setPageTitle(title)
}, { immediate: true })

function handleUserCommand(cmd) {
  if (cmd === 'profile') {
    router.push('/profile')
  } else if (cmd === 'logout') {
    authStore.logout()
  }
}
</script>

<style scoped>
.main-layout {
  display: flex;
  height: 100vh;
  overflow: hidden;
}

/* ===== Sidebar ===== */
.sidebar {
  width: var(--sidebar-width);
  min-width: var(--sidebar-width);
  background: var(--bg-sidebar);
  display: flex;
  flex-direction: column;
  transition: width var(--transition-normal), min-width var(--transition-normal);
  overflow: hidden;
  position: relative;
}

.sidebar::before {
  content: '';
  position: absolute;
  top: 0;
  right: 0;
  width: 1px;
  height: 100%;
  background: linear-gradient(to bottom, rgba(79, 110, 247, 0.25), rgba(124, 92, 252, 0.08), transparent);
  z-index: 1;
}

.sidebar-collapsed .sidebar {
  width: var(--sidebar-collapsed-width);
  min-width: var(--sidebar-collapsed-width);
}

.sidebar-logo {
  height: var(--header-height);
  display: flex;
  align-items: center;
  justify-content: center;
  position: relative;
  overflow: hidden;
  border-bottom: 1px solid rgba(255, 255, 255, 0.04);
}

.logo-glow {
  position: absolute;
  top: -30px;
  left: 50%;
  transform: translateX(-50%);
  width: 160px;
  height: 80px;
  background: radial-gradient(ellipse, rgba(79, 110, 247, 0.15), transparent 70%);
  pointer-events: none;
  animation: logoPulse 4s ease-in-out infinite;
}

@keyframes logoPulse {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.5; }
}

.logo-text {
  font-size: 17px;
  font-weight: 700;
  color: #fff;
  letter-spacing: 0.5px;
  position: relative;
  z-index: 1;
  background: linear-gradient(135deg, #fff 0%, rgba(255, 255, 255, 0.8) 100%);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
}

.logo-icon {
  font-size: 20px;
  font-weight: 800;
  background: var(--gradient-primary);
  -webkit-background-clip: text;
  -webkit-text-fill-color: transparent;
  background-clip: text;
}

.menu-group-title {
  padding: 16px 20px 6px;
  font-size: 10px;
  color: rgba(255, 255, 255, 0.2);
  text-transform: uppercase;
  letter-spacing: 2px;
  font-weight: 700;
}

.sidebar :deep(.el-menu) {
  border-right: none;
  flex: 1;
  overflow-y: auto;
  overflow-x: hidden;
  padding: 6px 0;
}

.sidebar :deep(.el-menu-item) {
  height: 44px;
  line-height: 44px;
  margin: 2px 12px;
  border-radius: var(--radius-sm);
  font-size: 13px;
  font-weight: 500;
  letter-spacing: 0.2px;
  transition: all var(--transition-fast);
}

.sidebar :deep(.el-menu-item:hover) {
  background: var(--bg-sidebar-hover) !important;
}

.sidebar :deep(.el-menu-item.is-active) {
  background: var(--bg-sidebar-active) !important;
  box-shadow: 0 2px 12px rgba(79, 110, 247, 0.25);
  font-weight: 600;
}

.sidebar :deep(.el-divider--horizontal) {
  margin: 8px 16px;
  border-color: rgba(255, 255, 255, 0.04);
}

/* ===== Main Area ===== */
.main-area {
  flex: 1;
  display: flex;
  flex-direction: column;
  min-width: 0;
}

/* ===== Header ===== */
.header {
  height: var(--header-height);
  min-height: var(--header-height);
  background: var(--glass-bg);
  backdrop-filter: var(--glass-blur);
  -webkit-backdrop-filter: var(--glass-blur);
  border-bottom: 1px solid var(--border-light);
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 0 28px;
  position: sticky;
  top: 0;
  z-index: 10;
}

.header-left {
  display: flex;
  align-items: center;
  gap: 14px;
}

.header-right {
  display: flex;
  align-items: center;
  gap: 14px;
}

.collapse-btn {
  font-size: 18px;
  color: var(--text-secondary);
  transition: color var(--transition-fast);
}
.collapse-btn:hover { color: var(--color-primary); }

.user-info {
  display: flex;
  align-items: center;
  gap: 10px;
  cursor: pointer;
  padding: 6px 12px;
  border-radius: var(--radius-md);
  transition: all var(--transition-fast);
}
.user-info:hover { background: var(--color-primary-bg); }

.username {
  font-size: 13px;
  color: var(--text-primary);
  max-width: 110px;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
  font-weight: 500;
}

.arrow {
  font-size: 12px;
  color: var(--text-secondary);
  transition: transform var(--transition-fast);
}

/* ===== Content ===== */
.content {
  flex: 1;
  overflow-y: auto;
  background: var(--bg-primary);
  background-image: var(--gradient-mesh);
  background-attachment: fixed;
}
</style>
